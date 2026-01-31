//
// Created by a.durynin on 29.01.2026.
//

#include "composition.h"

namespace tf {
  // RenderContext implementation
  RenderContext &RenderContext::with_param(const std::string &name, ParamValue value) {
    params[name] = std::move(value);
    return *this;
  }

  RenderContext &RenderContext::with_language(std::string lang) {
    targetLanguage = std::move(lang);
    return *this;
  }

  RenderContext &RenderContext::with_strict_mode(bool strict) {
    strictMode = strict;
    return *this;
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

  // Composition implementation
  Composition::Composition(CompositionId id) : id_(std::move(id)) {
  }

  const CompositionId &Composition::id() const noexcept { return id_; }
  BlockState Composition::state() const noexcept { return state_; }
  const Version &Composition::version() const noexcept { return version_; }
  const std::vector<Fragment> &Composition::fragments() const noexcept { return fragments_; }
  const std::optional<StyleProfile> &Composition::style_profile() const noexcept { return style_profile_; }
  const std::string &Composition::project_key() const noexcept { return project_key_; }
  const std::string &Composition::description() const noexcept { return description_; }

  void Composition::set_id(CompositionId id) { id_ = std::move(id); }

  void Composition::set_state(const BlockState state) {
    state_ = state;
  }

  void Composition::set_style_profile(StyleProfile profile) { style_profile_ = std::move(profile); }
  void Composition::set_project_key(std::string key) { project_key_ = std::move(key); }
  void Composition::set_description(std::string desc) { description_ = std::move(desc); }

  void Composition::set_version(const Version &v) { version_ = v; }

  Fragment &Composition::fragment(size_t index) { return fragments_.at(index); }
  const Fragment &Composition::fragment(size_t index) const { return fragments_.at(index); }
  size_t Composition::fragmentCount() const noexcept { return fragments_.size(); }

  Fragment &Composition::add_block_ref(const BlockId &blockId, Version version, Params localParams) {
    BlockRef ref(blockId, version, std::move(localParams));
    fragments_.emplace_back(Fragment::make_block_ref(std::move(ref)));
    return fragments_.back();
  }

  Fragment &Composition::add_block_ref_latest(const BlockId &blockId, Params localParams) {
    BlockRef ref(blockId);
    ref.set_local_params(std::move(localParams));
    fragments_.emplace_back(Fragment::make_block_ref(std::move(ref)));
    return fragments_.back();
  }

  Fragment &Composition::add_static_text(std::string text) {
    fragments_.emplace_back(Fragment::make_static_text(std::move(text)));
    return fragments_.back();
  }

  Fragment &Composition::add_separator(SeparatorType type) {
    fragments_.emplace_back(Fragment::make_separator(type));
    return fragments_.back();
  }

  void Composition::insert_fragment(size_t index, Fragment fragment) {
    if (index <= fragments_.size()) {
      fragments_.insert(fragments_.begin() + static_cast<std::ptrdiff_t>(index), std::move(fragment));
    }
  }

  void Composition::remove_fragment(size_t index) {
    if (index < fragments_.size()) {
      fragments_.erase(fragments_.begin() + static_cast<std::ptrdiff_t>(index));
    }
  }

  void Composition::clear_fragments() {
    fragments_.clear();
  }

  Error Composition::validate() const {
    if (id_.empty()) {
      return Error{ErrorCode::InvalidParamType, "Composition must have an ID"};
    }

    // Validate all fragments
    for (const auto &fragment: fragments_) {
      auto err = fragment.validate(state_ == BlockState::Draft);
      if (err.is_error()) {
        return err;
      }
    }

    return Error::success();
  }

  Error Composition::publish(Version new_version) {
    if (state_ != BlockState::Draft) {
      return Error{
        ErrorCode::InvalidStateTransition,
        "Only Draft compositions can be published"
      };
    }

    // Validate before publishing
    auto err = validate();
    if (err.is_error()) {
      return err;
    }

    // Check all BlockRefs have versions (not use_latest)
    for (const auto &fragment: fragments_) {
      if (fragment.is_block_ref()) {
        const auto &blockRef = fragment.as_block_ref();
        if (blockRef.use_latest()) {
          return Error::version_required();
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

  CompositionDraft::CompositionDraft(Composition &&c) : internal_(std::move(c)) {
  }

  CompositionId PublishedComposition::id() const noexcept { return id_; }

  Version PublishedComposition::version() const noexcept { return version_; }

  PublishedComposition::PublishedComposition(CompositionId id, Version ver) : id_(std::move(id)), version_(ver) {
  }

  // CompositionDraftBuilder implementation
  CompositionDraftBuilder::CompositionDraftBuilder(CompositionId id) : comp_(std::move(id)) {
  }

  CompositionDraftBuilder &CompositionDraftBuilder::with_id(CompositionId id) {
    comp_.set_id(std::move(id));
    return *this;
  }

  CompositionDraftBuilder &CompositionDraftBuilder::with_style_profile(StyleProfile profile) {
    comp_.set_style_profile(std::move(profile));
    return *this;
  }

  CompositionDraftBuilder &CompositionDraftBuilder::with_project_key(std::string key) {
    comp_.set_project_key(std::move(key));
    return *this;
  }

  CompositionDraftBuilder &CompositionDraftBuilder::with_description(std::string desc) {
    comp_.set_description(std::move(desc));
    return *this;
  }

  CompositionDraftBuilder &CompositionDraftBuilder::add_block_ref(BlockRef ref) {
    comp_.add_block_ref(ref.block_id(),
                        ref.version().value_or(Version{0, 0}),
                        ref.local_params());
    return *this;
  }

  CompositionDraftBuilder &CompositionDraftBuilder::add_static_text(std::string text) {
    comp_.add_static_text(std::move(text));
    return *this;
  }

  CompositionDraftBuilder &CompositionDraftBuilder::add_separator(SeparatorType type) {
    comp_.add_separator(type);
    return *this;
  }

  CompositionDraft CompositionDraftBuilder::build() {
    return CompositionDraft(std::move(comp_));
  }
} // namespace tf
