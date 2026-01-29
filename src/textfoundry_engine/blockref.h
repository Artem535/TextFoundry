//
// Created by artem.d on 28.01.2026.
//

#pragma once

#include "block.h"
#include "version.h"
#include "error.h"
#include "blocktype.hpp"

#include <optional>

namespace tf {

/**
 * BlockRef - reference to a published Block with local parameter overrides
 * Version is mandatory for reproducibility (use_latest only allowed in Draft Composition)
 */
class BlockRef {
public:
    BlockRef() = default;

    explicit BlockRef(BlockId blockId)
        : blockId_(std::move(blockId)), useLatest_(true) {}

    BlockRef(BlockId blockId, Version version)
        : blockId_(std::move(blockId)), version_(version), useLatest_(false) {}

    BlockRef(BlockId blockId, Version version, Params localParams)
        : blockId_(std::move(blockId)),
          version_(version),
          localParams_(std::move(localParams)),
          useLatest_(false) {}

    // Getters
    [[nodiscard]] const BlockId& blockId() const noexcept { return blockId_; }
    [[nodiscard]] const std::optional<Version>& version() const noexcept { return version_; }
    [[nodiscard]] const Params& localParams() const noexcept { return localParams_; }
    [[nodiscard]] bool useLatest() const noexcept { return useLatest_; }

    // Setters
    void setBlockId(BlockId id) { blockId_ = std::move(id); }
    void setVersion(Version ver) {
        version_ = ver;
        useLatest_ = false;
    }
    void setLocalParams(Params params) { localParams_ = std::move(params); }

    /**
     * Set use_latest flag (only allowed in Draft Composition)
     */
    void setUseLatest(bool useLatest) {
        useLatest_ = useLatest;
        if (useLatest) {
            version_.reset();
        }
    }

    /**
     * Add a local parameter override
     */
    BlockRef& withParam(const std::string& name, ParamValue value) {
        localParams_[name] = std::move(value);
        return *this;
    }

    /**
     * Validate this BlockRef
     * @param isDraftContext true if within Draft Composition (allows use_latest)
     * @returns Error if invalid (e.g., version required but not specified)
     */
    [[nodiscard]] Error validate(bool isDraftContext = false) const {
        if (blockId_.empty()) {
            return Error{ErrorCode::InvalidParamType, "BlockRef must have a blockId"};
        }
        if (!isDraftContext && useLatest_) {
            return Error::versionRequired();
        }
        return Error::success();
    }

    /**
     * Resolve all parameters for this block reference using hierarchy:
     * 1. Block Defaults (lowest priority)
     * 2. Local Override (this BlockRef)
     * 3. Runtime Context (highest priority)
     *
     * @param block The referenced block (must be loaded from storage)
     * @param runtimeContext Runtime parameters
     * @returns Resolved parameters or Error
     */
    [[nodiscard]] Result<Params> resolveParams(
        const Block& block,
        const Params& runtimeContext
    ) const;

private:
    BlockId blockId_;
    std::optional<Version> version_;
    Params localParams_;
    bool useLatest_ = true;  ///< If true, use latest version (Draft only)
};

} // namespace tf
