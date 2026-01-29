//
// Created by artem.d on 28.01.2026.
//

#include "block.h"

#include <ranges>
#include <regex>

#include <ctre-unicode.hpp>

namespace tf {
  // Template implementation
  std::vector<std::string> Template::extractParamNames() const {
    // Regex to match {{paramName}} pattern
    const auto names = ctre::search_all<R"(\{\{(\w+)\}\})">(content_)
                       | std::views::transform([](const auto &match) {
                         return match.template get<1>().to_string();
                       })
                       | std::ranges::to<std::vector<std::string> >();;
    return names;
  }

  Result<std::string> Template::expand(const Params &params) const {
    std::string result;
    result.reserve(content_.size() * 2);

    size_t last_pos = 0;
    for (const auto match: ctre::search_all<R"(\{\{(\w+)\}\})">(content_)) {
      const std::string_view paramName = match.template get<1>().to_view();

      auto count_symb = match.to_view().data() - content_.data() - last_pos;
      result.append(content_, last_pos, count_symb);

      if (!params.contains(paramName)) {
        return Result<std::string>(Error::missingParam(paramName));
      }

      result += params.at(paramName);
      last_pos = match.to_view().data() - content_.data() + match.to_view().size();
    }

    result += content_.substr(last_pos);
    return Result(result);
  }

  // Block implementation
  Result<ParamValue> Block::resolveParam(
    const std::string &paramName,
    const Params &localOverride,
    const Params &runtimeContext
  ) const {
    // 1. Check runtime context (highest priority)
    if (const auto runtimeIt = runtimeContext.find(paramName); runtimeIt != runtimeContext.end()) {
      return Result{runtimeIt->second};
    }

    // 2. Check local override
    if (const auto localIt = localOverride.find(paramName); localIt != localOverride.end()) {
      return Result{localIt->second};
    }

    // 3. Check block defaults (lowest priority)
    if (const auto defaultIt = defaults_.find(paramName); defaultIt != defaults_.end()) {
      return Result{defaultIt->second};
    }

    // Parameter not found
    return Result<ParamValue>(Error::missingParam(paramName));
  }

  Error Block::validateParams(
    const Params &localOverride,
    const Params &runtimeContext
  ) const {
    // Check all parameters in template have values
    const auto paramNames = template_.extractParamNames();
    for (const auto &paramName: paramNames) {
      if (!canResolveParam(paramName, localOverride, runtimeContext)) {
        return Error::missingParam(paramName);
      }
    }
    return Error::success();
  }

  Error Block::publish(const Version &newVersion) {
    if (state_ != BlockState::Draft) {
      return Error{
        ErrorCode::InvalidStateTransition,
        "Only Draft blocks can be published"
      };
    }
    if (id_.empty()) {
      return Error{ErrorCode::InvalidParamType, "Block must have an ID"};
    }
    if (template_.content().empty()) {
      return Error{ErrorCode::InvalidParamType, "Block must have a template"};
    }

    state_ = BlockState::Published;
    version_ = newVersion;
    return Error::success();
  }

  void Block::deprecate() {
    if (state_ == BlockState::Published) {
      state_ = BlockState::Deprecated;
    }
  }

  bool Block::canResolveParam(
    const std::string &paramName,
    const Params &localOverride,
    const Params &runtimeContext
  ) const {
    // Check runtime context
    if (runtimeContext.contains(paramName)) {
      return true;
    }

    // Check local override
    if (localOverride.contains(paramName)) {
      return true;
    }

    // Check block defaults
    if (defaults_.contains(paramName)) {
      return true;
    }

    return false;
  }

  BlockDraftBuilder::BlockDraftBuilder(BlockId id) : block_(std::move(id)) {
  }

  BlockDraftBuilder &BlockDraftBuilder::withId(BlockId id) {
    block_.setId(std::move(id));
    return *this;
  }

  BlockDraftBuilder &BlockDraftBuilder::withType(const BlockType &type) {
    block_.setType(type);
    return *this;
  }

  BlockDraftBuilder &BlockDraftBuilder::withTemplate(Template templ) {
    block_.setTemplate(std::move(templ));
    return *this;
  }

  BlockDraftBuilder &BlockDraftBuilder::withDefault(const std::string &name, ParamValue value) {
    auto defaults = block_.defaults();
    defaults[name] = std::move(value);
    block_.setDefaults(std::move(defaults));
    return *this;
  }

  BlockDraftBuilder &BlockDraftBuilder::withDefaults(Params defaults) {
    block_.setDefaults(std::move(defaults));
    return *this;
  }

  BlockDraftBuilder &BlockDraftBuilder::withTag(const std::string &tag) {
    auto tags = block_.tags();
    tags.insert(tag);
    block_.setTags(std::move(tags));
    return *this;
  }

  BlockDraftBuilder &BlockDraftBuilder::withLanguage(std::string lang) {
    block_.setLanguage(std::move(lang));
    return *this;
  }

  BlockDraftBuilder &BlockDraftBuilder::withDescription(std::string desc) {
    block_.setDescription(std::move(desc));
    return *this;
  }

  Block BlockDraftBuilder::build() const {
    return block_;
  }
} // namespace tf
