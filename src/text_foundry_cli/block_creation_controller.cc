#include "block_creation_controller.h"

#include <fmt/format.h>

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <vector>

namespace tf {
namespace {

inline void LTrim(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

inline void RTrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

inline void Trim(std::string& s) {
  LTrim(s);
  RTrim(s);
}

Params ParseParamsRaw(const std::string& raw) {
  Params params;
  std::size_t pos = 0;
  while (pos < raw.size()) {
    while (pos < raw.size() && (raw[pos] == ' ' || raw[pos] == ',')) pos++;
    if (pos >= raw.size()) break;

    const std::size_t eq = raw.find('=', pos);
    if (eq == std::string::npos) {
      throw std::invalid_argument("defaults format: key=value, key2=value2");
    }

    std::size_t comma = raw.find(',', eq + 1);
    if (comma == std::string::npos) comma = raw.size();

    std::string key = raw.substr(pos, eq - pos);
    std::string val = raw.substr(eq + 1, comma - (eq + 1));
    Trim(key);
    Trim(val);
    if (key.empty()) {
      throw std::invalid_argument("defaults key cannot be empty");
    }
    params[key] = val;
    pos = comma + 1;
  }
  return params;
}

std::vector<std::string> ParseTagsRaw(const std::string& raw) {
  std::vector<std::string> tags;
  std::size_t pos = 0;
  while (pos < raw.size()) {
    while (pos < raw.size() && (raw[pos] == ' ' || raw[pos] == ',')) pos++;
    if (pos >= raw.size()) break;

    std::size_t comma = raw.find(',', pos);
    if (comma == std::string::npos) comma = raw.size();

    std::string tag = raw.substr(pos, comma - pos);
    Trim(tag);
    if (!tag.empty()) tags.push_back(std::move(tag));
    pos = comma + 1;
  }
  return tags;
}

const std::vector<BlockType>& BlockTypes() {
  static const std::vector<BlockType> values = {
      BlockType::Role, BlockType::Constraint, BlockType::Style,
      BlockType::Domain, BlockType::Meta};
  return values;
}

int BlockTypeToIndex(const BlockType type) {
  const auto& block_types = BlockTypes();
  auto it = std::find(block_types.begin(), block_types.end(), type);
  if (it == block_types.end()) return 3;
  return static_cast<int>(std::distance(block_types.begin(), it));
}

std::string ParamsToRaw(const Params& params) {
  // Keep deterministic order for stable UX in the edit form.
  std::vector<std::pair<std::string, std::string>> ordered;
  ordered.reserve(params.size());
  for (const auto& [key, value] : params) {
    ordered.emplace_back(key, value);
  }
  std::sort(
      ordered.begin(), ordered.end(),
      [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

  std::string out;
  for (size_t i = 0; i < ordered.size(); ++i) {
    if (i > 0) out += ", ";
    out += ordered[i].first;
    out += "=";
    out += ordered[i].second;
  }
  return out;
}

std::string TagsToRaw(const std::unordered_set<std::string>& tags) {
  // Keep deterministic order for stable UX in the edit form.
  std::vector<std::string> ordered(tags.begin(), tags.end());
  std::sort(ordered.begin(), ordered.end());

  std::string out;
  for (size_t i = 0; i < ordered.size(); ++i) {
    if (i > 0) out += ", ";
    out += ordered[i];
  }
  return out;
}

}  // namespace

BlockCreationController::BlockCreationController(Engine& engine)
    : engine_(engine) {}

std::string BlockCreationController::ModalTitle() const {
  return mode_ == Mode::kEdit ? " Edit Block " : " Create Block ";
}

std::string BlockCreationController::SubmitButtonLabel() const {
  return mode_ == Mode::kEdit ? "Save Version" : "Create";
}

void BlockCreationController::BeginCreate() {
  mode_ = Mode::kCreate;
  editing_block_id_.clear();
  Reset("Fill in fields and press Create");
}

void BlockCreationController::BeginEdit(const Block& block) {
  mode_ = Mode::kEdit;
  editing_block_id_ = block.Id();
  block_id_ = block.Id();
  block_template_ = block.templ().Content();
  // Convert structured values back to editable text fields.
  block_defaults_ = ParamsToRaw(block.defaults());
  block_tags_ = TagsToRaw(block.tags());
  block_description_ = block.description();
  block_language_ = block.language();
  block_type_index_ = BlockTypeToIndex(block.type());
  status_ = fmt::format("Editing {}@{}.{} -> publish next version", block.Id(),
                        block.version().major, block.version().minor);
}

void BlockCreationController::Reset(const std::string& status) {
  block_id_.clear();
  block_template_.clear();
  block_defaults_.clear();
  block_tags_.clear();
  block_description_.clear();
  block_language_ = "en";
  block_type_index_ = 3;
  status_ = status;
  // Block ID must stay stable while editing; changing ID means creating a new
  // block.
  if (mode_ == Mode::kEdit && !editing_block_id_.empty()) {
    block_id_ = editing_block_id_;
  }
}

std::optional<tf::BlockId> BlockCreationController::Create() {
  try {
    std::string block_id = mode_ == Mode::kEdit ? editing_block_id_ : block_id_;
    std::string block_template = block_template_;
    std::string block_lang = block_language_;
    Trim(block_id);
    Trim(block_template);
    Trim(block_lang);

    if (block_id.empty()) {
      status_ = "Error: block id is required";
      return std::nullopt;
    }
    if (block_template.empty()) {
      status_ = "Error: template is required";
      return std::nullopt;
    }

    const auto& block_types = BlockTypes();
    if (block_type_index_ < 0 ||
        block_type_index_ >= static_cast<int>(block_types.size())) {
      status_ = "Error: invalid block type";
      return std::nullopt;
    }

    if (block_lang.empty()) block_lang = "en";

    auto builder = BlockDraftBuilder(block_id)
                       .WithTemplate(Template(block_template))
                       .WithType(block_types[block_type_index_])
                       .WithLanguage(block_lang);

    auto defaults = ParseParamsRaw(block_defaults_);
    if (!defaults.empty()) {
      builder.WithDefaults(std::move(defaults));
    }

    for (const auto& tag : ParseTagsRaw(block_tags_)) {
      builder.WithTag(tag);
    }

    if (!block_description_.empty()) {
      builder.WithDescription(block_description_);
    }

    auto draft = builder.build();
    // Edit always creates the next version of existing ID, create publishes new
    // ID.
    auto result =
        mode_ == Mode::kEdit
            ? engine_.UpdateBlock(std::move(draft), Engine::VersionBump::Minor)
            : engine_.PublishBlock(std::move(draft),
                                   Engine::VersionBump::Minor);
    if (result.HasError()) {
      status_ = "Error: " + result.error().message;
      return std::nullopt;
    }

    const auto& pub = result.value();
    status_ = fmt::format("Saved: {}@{}", pub.id(), pub.version().ToString());

    if (mode_ == Mode::kEdit) {
      editing_block_id_ = pub.id();
      block_id_ = pub.id();
    } else {
      block_id_.clear();
      block_template_.clear();
      block_defaults_.clear();
      block_tags_.clear();
      block_description_.clear();
    }

    return pub.id();
  } catch (const std::exception& ex) {
    status_ = "Error: " + std::string(ex.what());
    return std::nullopt;
  }
}

}  // namespace tf
