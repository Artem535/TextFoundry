//
// Created by a.durynin on 29.01.2026.
//

#include "composition.h"

namespace tf {
// RenderContext implementation
RenderContext& RenderContext::WithParam(const std::string& name,
                                         ParamValue value) {
  params[name] = std::move(value);
  return *this;
}

RenderContext& RenderContext::WithLanguage(std::string lang) {
  targetLanguage = std::move(lang);
  return *this;
}

RenderContext& RenderContext::with_strict_mode(bool strict) {
  strictMode = strict;
  return *this;
}

// SemanticStyle implementation
bool SemanticStyle::isEmpty() const noexcept {
  return !tone.has_value() && !tense.has_value() &&
         !targetLanguage.has_value() && !person.has_value() &&
         !rewriteStrength.has_value() && !audience.has_value() &&
         !locale.has_value() && !terminologyRigidity.has_value() &&
         !preserveFormatting.has_value() && !preserveExamples.has_value();
}

// StyleProfile implementation
StyleProfile StyleProfile::plain() { return StyleProfile{}; }

// Composition implementation
Composition::Composition(CompositionId id) : id_(std::move(id)) {}

const CompositionId& Composition::id() const noexcept { return id_; }
BlockState Composition::state() const noexcept { return state_; }
const Version& Composition::version() const noexcept { return version_; }
const std::vector<Fragment>& Composition::fragments() const noexcept {
  return fragments_;
}
const std::optional<StyleProfile>& Composition::GetStyleProfile() const noexcept {
  return style_profile_;
}
const std::string& Composition::ProjectKey() const noexcept {
  return project_key_;
}
const std::string& Composition::description() const noexcept {
  return description_;
}
const std::string& Composition::revision_comment() const noexcept {
  return revision_comment_;
}

void Composition::SetId(CompositionId id) { id_ = std::move(id); }

void Composition::SetState(const BlockState state) { state_ = state; }

void Composition::SetStyleProfile(StyleProfile profile) {
  style_profile_ = std::move(profile);
}
void Composition::SetProjectKey(std::string key) {
  project_key_ = std::move(key);
}
void Composition::SetDescription(std::string desc) {
  description_ = std::move(desc);
}
void Composition::SetRevisionComment(std::string comment) {
  revision_comment_ = std::move(comment);
}

void Composition::SetVersion(const Version& v) { version_ = v; }

Fragment& Composition::fragment(size_t index) { return fragments_.at(index); }
const Fragment& Composition::fragment(size_t index) const {
  return fragments_.at(index);
}
size_t Composition::fragmentCount() const noexcept { return fragments_.size(); }

Fragment& Composition::AddBlockRef(const BlockId& blockId, Version version,
                                     Params localParams) {
  BlockRef ref(blockId, version, std::move(localParams));
  fragments_.emplace_back(Fragment::MakeBlockRef(std::move(ref)));
  return fragments_.back();
}

Fragment& Composition::AddBlockRefLatest(const BlockId& blockId,
                                            Params localParams) {
  BlockRef ref(blockId);
  ref.SetLocalParams(std::move(localParams));
  fragments_.emplace_back(Fragment::MakeBlockRef(std::move(ref)));
  return fragments_.back();
}

Fragment& Composition::AddStaticText(std::string text) {
  fragments_.emplace_back(Fragment::MakeStaticText(std::move(text)));
  return fragments_.back();
}

Fragment& Composition::AddSeparator(SeparatorType type) {
  fragments_.emplace_back(Fragment::MakeSeparator(type));
  return fragments_.back();
}

void Composition::InsertFragment(size_t index, Fragment fragment) {
  if (index <= fragments_.size()) {
    fragments_.insert(fragments_.begin() + static_cast<std::ptrdiff_t>(index),
                      std::move(fragment));
  }
}

void Composition::RemoveFragment(size_t index) {
  if (index < fragments_.size()) {
    fragments_.erase(fragments_.begin() + static_cast<std::ptrdiff_t>(index));
  }
}

void Composition::ClearFragments() { fragments_.clear(); }

Error Composition::validate() const {
  if (id_.empty()) {
    return Error{ErrorCode::InvalidParamType, "Composition must have an ID"};
  }

  // Validate all fragments
  for (const auto& fragment : fragments_) {
    auto err = fragment.validate(state_ == BlockState::Draft);
    if (err.is_error()) {
      return err;
    }
  }

  return Error::success();
}

Error Composition::publish(Version new_version) {
  if (state_ != BlockState::Draft) {
    return Error{ErrorCode::InvalidStateTransition,
                 "Only Draft compositions can be published"};
  }

  // Validate before publishing
  auto err = validate();
  if (err.is_error()) {
    return err;
  }

  // Check all BlockRefs have versions (not UseLatest)
  for (const auto& fragment : fragments_) {
    if (fragment.IsBlockRef()) {
      const auto& blockRef = fragment.AsBlockRef();
      if (blockRef.UseLatest()) {
        return Error::VersionRequired();
      }
    }
  }

  state_ = BlockState::Published;
  version_ = new_version;
  return Error::success();
}

void Composition::deprecate() {
  if (state_ == BlockState::Published) {
    state_ = BlockState::Deprecated;
  }
}

CompositionDraft::CompositionDraft(Composition&& c) : internal_(std::move(c)) {}

CompositionId PublishedComposition::id() const noexcept { return id_; }

Version PublishedComposition::version() const noexcept { return version_; }

PublishedComposition::PublishedComposition(CompositionId id, Version ver)
    : id_(std::move(id)), version_(ver) {}

// CompositionDraftBuilder implementation
CompositionDraftBuilder::CompositionDraftBuilder(CompositionId id)
    : comp_(std::move(id)) {}

CompositionDraftBuilder& CompositionDraftBuilder::WithId(CompositionId id) {
  comp_.SetId(std::move(id));
  return *this;
}

CompositionDraftBuilder& CompositionDraftBuilder::WithStyleProfile(
    StyleProfile profile) {
  comp_.SetStyleProfile(std::move(profile));
  return *this;
}

CompositionDraftBuilder& CompositionDraftBuilder::WithProjectKey(
    std::string key) {
  comp_.SetProjectKey(std::move(key));
  return *this;
}

CompositionDraftBuilder& CompositionDraftBuilder::WithDescription(
    std::string desc) {
  comp_.SetDescription(std::move(desc));
  return *this;
}

CompositionDraftBuilder& CompositionDraftBuilder::WithRevisionComment(
    std::string comment) {
  comp_.SetRevisionComment(std::move(comment));
  return *this;
}

CompositionDraftBuilder& CompositionDraftBuilder::AddBlockRef(BlockRef ref) {
  comp_.AddBlockRef(ref.GetBlockId(), ref.version().value_or(Version{0, 0}),
                    ref.LocalParams());
  return *this;
}

CompositionDraftBuilder& CompositionDraftBuilder::AddBlockRef(
    const BlockId& id, uint16_t major, uint16_t minor, Params params) {
  return AddBlockRef(BlockRef(id, Version{major, minor}, std::move(params)));
}

CompositionDraftBuilder& CompositionDraftBuilder::AddBlockRef(
    const PublishedBlock& block, Params params) {
  BlockRef ref = block.ref();
  ref.SetLocalParams(std::move(params));
  return AddBlockRef(std::move(ref));
}

CompositionDraftBuilder& CompositionDraftBuilder::AddStaticText(
    std::string text) {
  comp_.AddStaticText(std::move(text));
  return *this;
}

CompositionDraftBuilder& CompositionDraftBuilder::AddSeparator(
    SeparatorType type) {
  comp_.AddSeparator(type);
  return *this;
}

CompositionDraft CompositionDraftBuilder::build() {
  return CompositionDraft(std::move(comp_));
}
}  // namespace tf
