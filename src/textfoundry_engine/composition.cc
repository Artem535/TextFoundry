//
// Created by a.durynin on 29.01.2026.
//

#include "composition.h"

namespace tf {

Fragment& Composition::addBlockRef(const BlockId& blockId, Version version, Params localParams) {
    BlockRef ref(blockId, version, std::move(localParams));
    fragments_.emplace_back(Fragment::makeBlockRef(std::move(ref)));
    return fragments_.back();
}

Fragment& Composition::addBlockRefLatest(const BlockId& blockId, Params localParams) {
    BlockRef ref(blockId);
    ref.setLocalParams(std::move(localParams));
    fragments_.emplace_back(Fragment::makeBlockRef(std::move(ref)));
    return fragments_.back();
}

Fragment& Composition::addStaticText(std::string text) {
    fragments_.emplace_back(Fragment::makeStaticText(std::move(text)));
    return fragments_.back();
}

Fragment& Composition::addSeparator(SeparatorType type) {
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
    for (const auto& fragment : fragments_) {
        auto err = fragment.validate(state_ == BlockState::Draft);
        if (err.isError()) {
            return err;
        }
    }

    return Error::success();
}

Error Composition::publish(Version newVersion) {
    if (state_ != BlockState::Draft) {
        return Error{ErrorCode::InvalidStateTransition,
                     "Only Draft compositions can be published"};
    }

    // Validate before publishing
    auto err = validate();
    if (err.isError()) {
        return err;
    }

    // Check all BlockRefs have versions (not use_latest)
    for (const auto& fragment : fragments_) {
        if (fragment.isBlockRef()) {
            const auto& blockRef = fragment.asBlockRef();
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

Error Composition::deserializeFragments(const std::string& json) {
    auto result = FragmentJsonSerializer::deserializeList(json);
    if (result.hasError()) {
        return result.error();
    }
    fragments_ = std::move(result.value());
    return Error::success();
}

} // namespace tf
