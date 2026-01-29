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

    explicit BlockRef(BlockId blockId);

    BlockRef(BlockId blockId, Version version);

    BlockRef(BlockId blockId, Version version, Params localParams);

    // Getters
    [[nodiscard]] const BlockId &blockId() const noexcept;

    [[nodiscard]] const std::optional<Version> &version() const noexcept;

    [[nodiscard]] const Params &localParams() const noexcept;

    [[nodiscard]] bool useLatest() const noexcept;

    // Setters
    void setBlockId(BlockId id);

    void setVersion(Version ver);

    void setLocalParams(Params params);

    /**
     * Set use_latest flag (only allowed in Draft Composition)
     */
    void setUseLatest(bool useLatest);

    /**
     * Add a local parameter override
     */
    BlockRef &withParam(const std::string &name, ParamValue value);

    /**
     * Validate this BlockRef
     * @param isDraftContext true if within Draft Composition (allows use_latest)
     * @returns Error if invalid (e.g., version required but not specified)
     */
    [[nodiscard]] Error validate(bool isDraftContext = false) const;

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
      const Block &block,
      const Params &runtimeContext
    ) const;

  private:
    BlockId blockId_;
    std::optional<Version> version_;
    Params localParams_;
    bool useLatest_ = true; ///< If true, use latest version (Draft only)
    BlockId block_id_;
  };
} // namespace tf
