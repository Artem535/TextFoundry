//
// Created by artem.d on 30.01.2026.
//

#pragma once

#include <memory>
#include <objectbox.hpp>

#include "database_scheme.obx.hpp"
#include "engine.h"
#include "objectbox-model.h"

namespace tf {
/**
 * @brief ObjectBox-based implementation of IBlockRepository
 *
 * Provides persistent storage for Block entities using ObjectBox database.
 * Stores blocks with their metadata, templates, and parameters.
 *
 * Block entities are stored in the ObxBlock table with:
 * - Identity: blockId, version (major/minor)
 * - Content: template content, defaults as JSON
 * - Metadata: type, state, description, timestamps
 * - Relations: project, language, version chain (previous/next)
 *
 * @note Parameter schemas are stored separately in ObxBlockParam for
 * queryability.
 */
class BlockRepository final : public IBlockRepository {
 public:
  /**
   * @brief Constructs a BlockRepository with the given ObjectBox store
   * @param store Shared pointer to the ObjectBox store instance
   *
   * Initializes the project and block boxes for database operations.
   */
  explicit BlockRepository(std::shared_ptr<obx::Store> store);

  /**
   * @brief Saves a block to the database
   * @param block The block to save
   * @return Error::Success on success, or StorageError on failure
   *
   * Converts the domain Block to ObxBlock and stores it.
   * If a block with the same ID and version already exists, it will be
   * overwritten.
   */
  [[nodiscard]] Error save(const Block& block) override;

  /**
   * @brief Loads a specific version of a block
   * @param id The block identifier
   * @param version The specific version to load (major.minor)
   * @return Result containing the Block on success, or Error on failure
   *
   * Queries the database for a block matching both the ID and version.
   */
  [[nodiscard]] Result<Block> load(const BlockId& id, Version version) override;

  /**
   * @brief Loads the latest version of a block
   * @param id The block identifier
   * @return Result containing the Block on success, or Error on failure
   *
   * Finds the maximum version for the given block ID and loads it.
   * Convenience method that calls getLatestVersion() followed by load().
   */
  [[nodiscard]] Result<Block> LoadLatest(const BlockId& id) override;

  /**
   * @brief Lists all block IDs, optionally filtered by type
   * @param typeFilter Optional filter for block type (Role, Constraint, etc.)
   * @return Vector of block identifiers
   *
   * Returns all block IDs matching the filter criteria.
   * If no filter is provided, returns all blocks in the database.
   */
  [[nodiscard]] std::vector<BlockId> list(
      std::optional<BlockType> typeFilter) override;

  /**
   * @brief Gets the latest version number for a block
   * @param id The block identifier
   * @return Result containing the Version on success, or Error if not found
   *
   * Scans all versions of the block and returns the maximum
   * version number (comparing major first, then minor).
   */
  [[nodiscard]] Result<Version> GetLatestVersion(const BlockId& id) override;

  [[nodiscard]] Result<std::vector<Version>> ListVersions(
      const BlockId& id) override;

  /**
   * @brief Marks a specific block version as deprecated
   * @param id The block identifier
   * @param version The version to deprecate
   * @return Error::Success on success, or Error if not found
   *
   * Changes the state of the block to Deprecated.
   * Deprecated blocks remain in the database but should not be
   * used for new compositions.
   */
  [[nodiscard]] Error deprecate(const BlockId& id, Version version) override;
  [[nodiscard]] Error remove(const BlockId& id) override;

 private:
  std::shared_ptr<obx::Store> store_;  ///< ObjectBox store instance
  std::unique_ptr<obx::Box<ObxProject> > box_project_;  ///< Project entity box
  std::unique_ptr<obx::Box<ObxBlock> > box_block_;      ///< Block entity box
};

/**
 * @brief ObjectBox-based implementation of ICompositionRepository
 *
 * Provides persistent storage for Composition entities using ObjectBox
 * database. Stores compositions with their metadata, style profiles, and
 * fragment references.
 *
 * Composition entities are stored across two tables:
 * - ObxComposition: composition metadata (id, version, state, styles,
 * description)
 * - ObxFragment: individual fragments belonging to a composition
 *
 * @note Fragments are stored as separate entities to support large compositions
 *       and allow querying individual fragments if needed.
 */
class CompositionRepository final : public ICompositionRepository {
 public:
  /**
   * @brief Constructs a CompositionRepository with the given ObjectBox store
   * @param store Shared pointer to the ObjectBox store instance
   *
   * Initializes the composition box for database operations.
   */
  explicit CompositionRepository(std::shared_ptr<obx::Store> store);

  /**
   * @brief Saves a composition to the database
   * @param composition The composition to save
   * @return Error::Success on success, or StorageError on failure
   *
   * Converts the domain Composition to ObxComposition and stores it along with
   * its fragments. If a composition with the same ID and version already
   * exists, it will be overwritten (upsert behavior).
   */
  [[nodiscard]] Error save(const Composition& composition) override;

  /**
   * @brief Loads a specific version of a composition
   * @param id The composition identifier
   * @param version The specific version to load (major.minor)
   * @return Result containing the Composition on success, or Error on failure
   *
   * Queries the database for a composition matching both the ID and version.
   * Also loads all associated fragments in their proper order.
   */
  [[nodiscard]] Result<Composition> load(const CompositionId& id,
                                         Version version) override;

  /**
   * @brief Loads the latest version of a composition
   * @param id The composition identifier
   * @return Result containing the Composition on success, or Error on failure
   *
   * Finds the maximum version for the given composition ID and loads it.
   * Convenience method that calls getLatestVersion() followed by load().
   */
  [[nodiscard]] Result<Composition> LoadLatest(
      const CompositionId& id) override;

  /**
   * @brief Lists all composition IDs in the database
   * @return Vector of all unique composition identifiers
   *
   * Returns all composition IDs regardless of version or state.
   * Useful for browsing and management operations.
   */
  [[nodiscard]] std::vector<CompositionId> list() override;

  /**
   * @brief Gets the latest version number for a composition
   * @param id The composition identifier
   * @return Result containing the Version on success, or Error if not found
   *
   * Scans all versions of the composition and returns the maximum
   * version number (comparing major first, then minor).
   */
  [[nodiscard]] Result<Version> GetLatestVersion(
      const CompositionId& id) override;

  [[nodiscard]] Result<std::vector<Version>> ListVersions(
      const CompositionId& id) override;

  /**
   * @brief Marks a specific composition version as deprecated
   * @param id The composition identifier
   * @param version The version to deprecate
   * @return Error::Success on success, or Error if not found
   *
   * Changes the state of the composition to Deprecated.
   * Deprecated compositions remain in the database but should not be
   * used for new rendering operations.
   */
  [[nodiscard]] Error deprecate(const CompositionId& id,
                                Version version) override;
  [[nodiscard]] Error remove(const CompositionId& id) override;

 private:
  std::shared_ptr<obx::Store> store_;  ///< ObjectBox store instance
  std::unique_ptr<obx::Box<ObxComposition> >
      box_composition_;  ///< Composition entity box
  std::unique_ptr<obx::Box<ObxFragment> >
      box_fragment_;  ///< Fragment entity box
};
}  // namespace tf
