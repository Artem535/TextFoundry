//
// Created by artem.d on 28.01.2026.
//

#pragma once

#include <optional>

#include "blocktype.hpp"
#include "error.h"
#include "types.h"
#include "version.h"

namespace tf {
class Block;

/**
 * BlockRef - reference to a published Block with local parameter overrides
 * Version is mandatory for reproducibility (UseLatest only allowed in Draft
 * Composition)
 */
class BlockRef {
 public:
  BlockRef() = default;

  explicit BlockRef(BlockId blockId);

  BlockRef(BlockId blockId, Version version);

  BlockRef(BlockId blockId, Version version, Params LocalParams);

  // Getters
  [[nodiscard]] const BlockId& GetBlockId() const noexcept;

  [[nodiscard]] const std::optional<Version>& version() const noexcept;

  [[nodiscard]] const Params& LocalParams() const noexcept;

  [[nodiscard]] bool UseLatest() const noexcept;

  // Setters
  void SetBlockId(BlockId id);

  void SetVersion(Version ver);

  void SetLocalParams(Params params);

  /**
   * Set UseLatest flag (only allowed in Draft Composition)
   */
  void SetUseLatest(bool UseLatest);

  /**
   * Add a local parameter override
   */
  BlockRef& WithParam(const std::string& name, ParamValue value);

  /**
   * Validate this BlockRef
   * @param is_draft_context true if within Draft Composition (allows
   * UseLatest)
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
  [[nodiscard]] Result<Params> ResolveParams(
      const Block& block, const Params& runtime_context) const;

 private:
  BlockId blockId_;
  std::optional<Version> version_;
  Params localParams_;
  bool useLatest_ = true;  ///< If true, use latest version (Draft only)
};
}  // namespace tf
