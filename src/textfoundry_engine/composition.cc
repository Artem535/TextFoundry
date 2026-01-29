//
// Created by a.durynin on 29.01.2026.
//

#include "composition.h"

namespace tf {
  // RenderContext implementation
  RenderContext &RenderContext::withParam(const std::string &name, ParamValue value) {
    params[name] = std::move(value);
    return *this;
  }

  RenderContext &RenderContext::withLanguage(std::string lang) {
    targetLanguage = std::move(lang);
    return *this;
  }

  RenderContext &RenderContext::withStrictMode(bool strict) {
    strictMode = strict;
    return *this;
  }

  // StructuralStyle implementation
  StructuralStyle StructuralStyle::plain() {
    return StructuralStyle{};
  }

  StructuralStyle StructuralStyle::markdown() {
    StructuralStyle style;
    style.outputFormat = OutputFormat::Markdown;
    return style;
  }

  StructuralStyle StructuralStyle::json() {
    StructuralStyle style;
    style.outputFormat = OutputFormat::Json;
    return style;
  }

  // SemanticStyle implementation
  bool SemanticStyle::isEmpty() const noexcept {
    return !tone.has_value() &&
           !tense.has_value() &&
           !targetLanguage.has_value() &&
           !person.has_value();
  }

  // StyleProfile implementation
  StyleProfile StyleProfile::plain() {
    return StyleProfile{};
  }

  StyleProfile StyleProfile::markdown() {
    StyleProfile profile;
    profile.structural = StructuralStyle::markdown();
    return profile;
  }

  // Composition implementation
  Composition::Composition(CompositionId id) : id_(std::move(id)) {
  }

  const CompositionId &Composition::id() const noexcept { return id_; }
  BlockState Composition::state() const noexcept { return state_; }
  const Version &Composition::version() const noexcept { return version_; }
  const std::vector<Fragment> &Composition::fragments() const noexcept { return fragments_; }
  const std::optional<StyleProfile> &Composition::styleProfile() const noexcept { return styleProfile_; }
  const std::string &Composition::projectKey() const noexcept { return projectKey_; }
  const std::string &Composition::description() const noexcept { return description_; }

  void Composition::setId(CompositionId id) { id_ = std::move(id); }
  void Composition::setStyleProfile(StyleProfile profile) { styleProfile_ = std::move(profile); }
  void Composition::setProjectKey(std::string key) { projectKey_ = std::move(key); }
  void Composition::setDescription(std::string desc) { description_ = std::move(desc); }

  Fragment &Composition::fragment(size_t index) { return fragments_.at(index); }
  const Fragment &Composition::fragment(size_t index) const { return fragments_.at(index); }
  size_t Composition::fragmentCount() const noexcept { return fragments_.size(); }

  Fragment &Composition::addBlockRef(const BlockId &blockId, Version version, Params localParams) {
    BlockRef ref(blockId, version, std::move(localParams));
    fragments_.emplace_back(Fragment::makeBlockRef(std::move(ref)));
    return fragments_.back();
  }

  Fragment &Composition::addBlockRefLatest(const BlockId &blockId, Params localParams) {
    BlockRef ref(blockId);
    ref.setLocalParams(std::move(localParams));
    fragments_.emplace_back(Fragment::makeBlockRef(std::move(ref)));
    return fragments_.back();
  }

  Fragment &Composition::addStaticText(std::string text) {
    fragments_.emplace_back(Fragment::makeStaticText(std::move(text)));
    return fragments_.back();
  }

  Fragment &Composition::addSeparator(SeparatorType type) {
    fragments_.emplace_back(Fragment::makeSeparator(type));
    return fragments_.back();
  }

  void Composition::insertFragment(size_t index, Fragment fragment) {
    if (index <= fragments_.size()) {
      fragments_.insert(fragments_.begin() + static_cast<std::ptrdiff_t>(index), std::move(fragment));
    }
  }

  void Composition::removeFragment(size_t index) {
    if (index < fragments_.size()) {
      fragments_.erase(fragments_.begin() + static_cast<std::ptrdiff_t>(index));
    }
  }

  void Composition::clearFragments() {
    fragments_.clear();
  }

  Error Composition::validate() const {
    if (id_.empty()) {
      return Error{ErrorCode::InvalidParamType, "Composition must have an ID"};
    }

    // Validate all fragments
    for (const auto &fragment: fragments_) {
      auto err = fragment.validate(state_ == BlockState::Draft);
      if (err.isError()) {
        return err;
      }
    }

    return Error::success();
  }

  Error Composition::publish(Version newVersion) {
    if (state_ != BlockState::Draft) {
      return Error{
        ErrorCode::InvalidStateTransition,
        "Only Draft compositions can be published"
      };
    }

    // Validate before publishing
    auto err = validate();
    if (err.isError()) {
      return err;
    }

    // Check all BlockRefs have versions (not use_latest)
    for (const auto &fragment: fragments_) {
      if (fragment.isBlockRef()) {
        const auto &blockRef = fragment.asBlockRef();
        if (blockRef.useLatest()) {
          return Error::versionRequired();
        }
      }
    }

    state_ = BlockState::Published;
    version_ = newVersion;
    return Error::success();
  }

  void Composition::deprecate() {
    if (state_ == BlockState::Published) {
      state_ = BlockState::Deprecated;
    }
  }

  std::string Composition::serializeFragments() const {
    return FragmentJsonSerializer::serialize(fragments_);
  }

  Error Composition::deserializeFragments(const std::string &json) {
    auto result = FragmentJsonSerializer::deserializeList(json);
    if (result.hasError()) {
      return result.error();
    }
    fragments_ = std::move(result.value());
    return Error::success();
  }

  // CompositionDraftBuilder implementation
  CompositionDraftBuilder::CompositionDraftBuilder(CompositionId id) : comp_(std::move(id)) {
  }

  CompositionDraftBuilder &CompositionDraftBuilder::withId(CompositionId id) {
    comp_.setId(std::move(id));
    return *this;
  }

  CompositionDraftBuilder &CompositionDraftBuilder::withStyleProfile(StyleProfile profile) {
    comp_.setStyleProfile(std::move(profile));
    return *this;
  }

  CompositionDraftBuilder &CompositionDraftBuilder::withProjectKey(std::string key) {
    comp_.setProjectKey(std::move(key));
    return *this;
  }

  CompositionDraftBuilder &CompositionDraftBuilder::withDescription(std::string desc) {
    comp_.setDescription(std::move(desc));
    return *this;
  }

  CompositionDraftBuilder &CompositionDraftBuilder::addBlockRef(const BlockId &blockId, Version version,
                                                                Params localParams) {
    comp_.addBlockRef(blockId, version, std::move(localParams));
    return *this;
  }

  CompositionDraftBuilder &CompositionDraftBuilder::addStaticText(std::string text) {
    comp_.addStaticText(std::move(text));
    return *this;
  }

  CompositionDraftBuilder &CompositionDraftBuilder::addSeparator(SeparatorType type) {
    comp_.addSeparator(type);
    return *this;
  }

  Composition CompositionDraftBuilder::build() const {
    return comp_;
  }
} // namespace tf
