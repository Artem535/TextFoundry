//
// Created by artem.d on 28.01.2026.
//

#include "block.h"

#include <ctre-unicode.hpp>
#include <ranges>
#include <regex>

#include "blockref.h"

namespace tf {
// Template implementation
std::vector<std::string> Template::extract_param_names() const {
  // Regex to match {{paramName}} pattern
  const auto names = ctre::search_all<R"(\{\{(\w+)\}\})">(content_) |
                     std::views::transform([](const auto& match) {
                       return match.template get<1>().to_string();
                     }) |
                     std::ranges::to<std::vector<std::string> >();
  ;
  return names;
}

Result<std::string> Template::expand(const Params& params) const {
  std::string result;
  result.reserve(content_.size() * 2);

  size_t last_pos = 0;
  for (const auto match : ctre::search_all<R"(\{\{(\w+)\}\})">(content_)) {
    const std::string paramName = match.template get<1>().to_string();

    auto count_symb = match.to_view().data() - content_.data() - last_pos;
    result.append(content_, last_pos, count_symb);

    if (!params.contains(paramName)) {
      return Result<std::string>(Error::missing_param(paramName));
    }

    result += params.at(paramName);
    last_pos =
        match.to_view().data() - content_.data() + match.to_view().size();
  }

  result += content_.substr(last_pos);
  return Result(result);
}

const BlockId& Block::id() const noexcept { return id_; }

BlockType Block::type() const noexcept { return type_; }

BlockState Block::state() const noexcept { return state_; }

const Version& Block::version() const noexcept { return version_; }

// Block implementation
Result<ParamValue> Block::resolve_param(const std::string& param_name,
                                        const Params& local_override,
                                        const Params& runtime_context) const {
  // 1. Check runtime context (highest priority)
  if (const auto runtimeIt = runtime_context.find(param_name);
      runtimeIt != runtime_context.end()) {
    return Result{runtimeIt->second};
  }

  // 2. Check local override
  if (const auto localIt = local_override.find(param_name);
      localIt != local_override.end()) {
    return Result{localIt->second};
  }

  // 3. Check block defaults (lowest priority)
  if (const auto defaultIt = defaults_.find(param_name);
      defaultIt != defaults_.end()) {
    return Result{defaultIt->second};
  }

  // Parameter not found
  return Result<ParamValue>(Error::missing_param(param_name));
}

Error Block::validate_params(const Params& local_override,
                             const Params& runtime_context) const {
  // Check all parameters in template have values
  const auto paramNames = template_.extract_param_names();
  for (const auto& paramName : paramNames) {
    if (!can_resolve_param(paramName, local_override, runtime_context)) {
      return Error::missing_param(paramName);
    }
  }
  return Error::success();
}

Error Block::publish(const Version& new_version) {
  if (state_ != BlockState::Draft) {
    return Error{ErrorCode::InvalidStateTransition,
                 "Only Draft blocks can be published"};
  }
  if (id_.empty()) {
    return Error{ErrorCode::InvalidParamType, "Block must have an ID"};
  }
  if (template_.content().empty()) {
    return Error{ErrorCode::InvalidParamType, "Block must have a template"};
  }

  state_ = BlockState::Published;
  version_ = new_version;
  return Error::success();
}

void Block::deprecate() {
  if (state_ == BlockState::Published) {
    state_ = BlockState::Deprecated;
  }
}

void Block::set_version(const Version& v) { version_ = v; }

const Template& Block::templ() const noexcept { return template_; }

const Params& Block::defaults() const noexcept { return defaults_; }

const std::vector<ParamSchema>& Block::param_schema() const noexcept {
  return paramSchema_;
}

const std::unordered_set<std::string>& Block::tags() const noexcept {
  return tags_;
}

const std::string& Block::language() const noexcept { return language_; }

const std::string& Block::description() const noexcept { return description_; }

void Block::set_id(BlockId id) { id_ = std::move(id); }

void Block::set_type(BlockType type) { type_ = type; }

void Block::set_state(BlockState state) { state_ = state; }

void Block::set_template(Template templ) { template_ = std::move(templ); }

void Block::set_defaults(Params defaults) { defaults_ = std::move(defaults); }

void Block::set_param_schema(std::vector<ParamSchema> schema) {
  paramSchema_ = std::move(schema);
}

void Block::set_tags(std::unordered_set<std::string> tags) {
  tags_ = std::move(tags);
}

void Block::set_language(std::string lang) { language_ = std::move(lang); }

void Block::set_description(std::string desc) {
  description_ = std::move(desc);
}

bool Block::can_resolve_param(const std::string& param_name,
                              const Params& local_override,
                              const Params& runtime_context) const {
  // Check runtime context
  if (runtime_context.contains(param_name)) {
    return true;
  }

  // Check local override
  if (local_override.contains(param_name)) {
    return true;
  }

  // Check block defaults
  if (defaults_.contains(param_name)) {
    return true;
  }

  return false;
}

BlockRef PublishedBlock::ref() const { return {id_, version_}; }

BlockDraftBuilder::BlockDraftBuilder(BlockId id) : block_(std::move(id)) {
  block_.set_state(BlockState::Draft);
}

BlockDraftBuilder& BlockDraftBuilder::with_id(BlockId id) {
  block_.set_id(std::move(id));
  return *this;
}

BlockDraftBuilder& BlockDraftBuilder::with_type(const BlockType& type) {
  block_.set_type(type);
  return *this;
}

BlockDraftBuilder& BlockDraftBuilder::with_template(Template templ) {
  block_.set_template(std::move(templ));
  return *this;
}

BlockDraftBuilder& BlockDraftBuilder::with_default(const std::string& name,
                                                   ParamValue value) {
  auto defaults = block_.defaults();
  defaults[name] = std::move(value);
  block_.set_defaults(std::move(defaults));
  return *this;
}

BlockDraftBuilder& BlockDraftBuilder::with_defaults(Params defaults) {
  block_.set_defaults(std::move(defaults));
  return *this;
}

BlockDraftBuilder& BlockDraftBuilder::with_tag(const std::string& tag) {
  auto tags = block_.tags();
  tags.insert(tag);
  block_.set_tags(std::move(tags));
  return *this;
}

BlockDraftBuilder& BlockDraftBuilder::with_language(std::string lang) {
  block_.set_language(std::move(lang));
  return *this;
}

BlockDraftBuilder& BlockDraftBuilder::with_description(std::string desc) {
  block_.set_description(std::move(desc));
  return *this;
}

BlockDraft BlockDraftBuilder::build() { return BlockDraft(std::move(block_)); }
}  // namespace tf
