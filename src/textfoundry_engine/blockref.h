//
// Created by artem.d on 28.01.2026.
//

#pragma once

#include "types.h"
#include "version.h"
#include "error.h"
#include "blocktype.hpp"

#include <optional>

namespace tf {
  class Block;

  /**
   * BlockRef - reference to a published Block with local parameter overrides
   * Version is mandatory for reproducibility (use_latest only allowed in Draft Composition)
   */
  class BlockRef {
  public:
    BlockRef() = default;

    explicit BlockRef(BlockId block_id);

    BlockRef(BlockId blockId, Version version);

    BlockRef(BlockId blockId, Version version, Params local_params);

    // Getters
    [[nodiscard]] const BlockId &block_id() const noexcept;

    [[nodiscard]] const std::optional<Version> &version() const noexcept;

    [[nodiscard]] const Params &local_params() const noexcept;

    [[nodiscard]] bool use_latest() const noexcept;

    // Setters
    void set_block_id(BlockId id);

    void set_version(Version ver);

    void set_local_params(Params params);

    /**
     * Set use_latest flag (only allowed in Draft Composition)
     */
    void set_use_latest(bool use_latest);

    /**
     * Add a local parameter override
     */
    BlockRef &with_param(const std::string &name, ParamValue value);

    /**
     * Validate this BlockRef
     * @param is_draft_context true if within Draft Composition (allows use_latest)
     * @returns Error if invalid (e.g., version required but not specified)
     */
    [[nodiscard]] Error validate(bool is_draft_context = false) const;

    /**
     * Resolve all parameters for this block reference using hierarchy:
     * 1. Block Defaults (lowest priority)
     * 2. Local Override (this BlockRef)
     * 3. Runtime Context (highest priority)
     *
     * @param block The referenced block (must be loaded from storage)
     * @param runtime_context Runtime parameters
     * @returns Resolved parameters or Error
     */
    [[nodiscard]] Result<Params> resolve_params(
      const Block &block,
      const Params &runtime_context
    ) const;

  private:
    BlockId blockId_;
    std::optional<Version> version_;
    Params localParams_;
    bool useLatest_ = true; ///< If true, use latest version (Draft only)
  };
} // namespace tf
