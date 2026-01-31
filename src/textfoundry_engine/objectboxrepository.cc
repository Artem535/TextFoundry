//
// Created by artem.d on 30.01.2026.
//

#define OBX_CPP_FILE
#include "objectboxrepository.h"

#include "obx_utils.hpp"
#include "logger.h"
#include <ranges>
#include <unordered_set>

namespace tf {
  BlockRepository::BlockRepository(std::shared_ptr<obx::Store> store) : store_(std::move(store)) {
    box_project_ = std::make_unique<obx::Box<ObxProject> >(*store_);
    box_block_ = std::make_unique<obx::Box<ObxBlock> >(*store_);
  }

  Error BlockRepository::save(const Block &block) {
    auto obx_block = utils::blockToObxBlock(block);

    // Store the block entity
    const obx_id result = box_block_->put(obx_block);
    if (result == 0) {
      TF_LOG_ERROR("BlockRepository::save| Failed to save block: {}", block.id());
      return Error{ErrorCode::StorageError, "Failed to save block"};
    }

    TF_LOG_INFO("BlockRepository::save| Block saved: {} v{}.{}",
                block.id(), block.version().major, block.version().minor);
    return Error{ErrorCode::Success, ""};
  }

  Result<Block> BlockRepository::load(const BlockId &id, const Version version) {
    auto query = box_block_->query(
      ObxBlock_::blockId.equals(id) &&
      ObxBlock_::versionMajor.equals(version.major) &&
      ObxBlock_::versionMinor.equals(version.minor)
    ).build();

    const auto block = query.findFirst();

    if (!block) {
      TF_LOG_ERROR("Block not found");
      return Result<Block>(Error{ErrorCode::StorageError, "Block not found"});
    }

    return Result(utils::obxBlockToBlock(*block));
  }

  Result<Block> BlockRepository::loadLatest(const BlockId &id) {
    const auto version = getLatestVersion(id);
    if (version.hasError()) {
      TF_LOG_ERROR("BlockRepository::loadLatest| Error msg: {}", version.error().message);
      return Result<Block>(version.error());
    }

    const auto loaded = load(id, version.value());
    return loaded;
  }

  std::vector<BlockId> BlockRepository::list(std::optional<BlockType> typeFilter) {
    // If no typeFilter is provided, return all blocks
    if (!typeFilter.has_value()) {
      const auto blockIds = box_block_->getAll() | std::views::transform([&](const auto &block) {
        return block->blockId;
      }) | std::ranges::to<std::set<BlockId> >();

      if (blockIds.empty()) {
        TF_LOG_WARN("BlockRepository::list| No blocks found");
      }

      return {blockIds.begin(), blockIds.end()};
    }
    // If typeFilter is provided, return blocks by type
    const auto obx_block_type = utils::blockTypeToObxType(*typeFilter);
    auto query = box_block_->query(ObxBlock_::type.equals(obx_block_type)).build();
    const auto objects = query.find();

    // If no blocks are found, return an empty vector
    if (objects.empty()) {
      TF_LOG_WARN("BlockRepository::list| Blocks by types not found");
    }

    const auto results_set = objects | std::views::transform([](const auto &block) {
      return block.blockId;
    }) | std::ranges::to<std::set<BlockId> >();

    return {results_set.begin(), results_set.end()};
  }


  Result<Version> BlockRepository::getLatestVersion(const BlockId &id) {
    auto query = box_block_->query(ObxBlock_::blockId.equals(id)).build();
    auto blocks = query.find();

    if (blocks.empty()) {
      TF_LOG_ERROR("BlockRepository::getLatestVersion| Block not found: {}", id);
      return Result<Version>(Error{ErrorCode::StorageError, "Block not found"});
    }

    const auto versions = blocks | std::views::transform([](const auto &block) {
      return Version{block.versionMajor, block.versionMinor};
    });
    const auto version = *std::ranges::max_element(versions);
    return Result{version};
  }

  Error BlockRepository::deprecate(const BlockId &id, Version version) {
    auto query = box_block_->query(
      ObxBlock_::blockId.equals(id) &&
      ObxBlock_::versionMajor.equals(version.major) &&
      ObxBlock_::versionMinor.equals(version.minor)
    ).build();

    const auto block = query.findFirst();
    if (!block) {
      TF_LOG_ERROR("BlockRepository::deprecate| Block not found");
      return Error{ErrorCode::StorageError, "Block not found"};
    }

    // Set state to Deprecated (2)
    block->state = utils::blockStateToObxStateCode(BlockState::Deprecated);

    if (!box_block_->put(*block)) {
      TF_LOG_ERROR("BlockRepository::deprecate| Failed to save block");
      return Error{ErrorCode::StorageError, "Failed to save block"};
    }

    TF_LOG_INFO("BlockRepository::deprecate| Block deprecated, block_id: {}", id);
    return Error{ErrorCode::Success};
  }

  // ============================================================================
  // CompositionRepository Implementation
  // ============================================================================

  CompositionRepository::CompositionRepository(std::shared_ptr<obx::Store> store) : store_(std::move(store)) {
    box_composition_ = std::make_unique<obx::Box<ObxComposition> >(*store_);
    box_fragment_ = std::make_unique<obx::Box<ObxFragment> >(*store_);
  }

  Error CompositionRepository::save(const Composition &composition) {
    // Convert domain Composition to ObxComposition
    auto obx_comp = utils::compositionToObxComposition(composition);
    obx_comp.state = utils::blockStateToObxStateCode(composition.state());

    // Save the composition entity
    const obx_id comp_id = box_composition_->put(obx_comp);
    if (comp_id == 0) {
      TF_LOG_ERROR("CompositionRepository::save| Failed to save composition");
      return Error{ErrorCode::StorageError, "Failed to save composition"};
    }

    // Delete existing fragments for this composition (if updating)
    auto frag_query = box_fragment_->query(ObxFragment_::compositionId.equals(comp_id)).build();
    auto existing_fragments = frag_query.find();
    for (const auto &frag: existing_fragments) {
      box_fragment_->remove(frag.id);
    }

    // Save fragments
    uint32_t order_index = 0;
    for (const auto &fragment: composition.fragments()) {
      auto obx_frag = utils::fragmentToObxFragment(fragment, order_index++);
      obx_frag.compositionId = comp_id;
      box_fragment_->put(obx_frag);
    }

    TF_LOG_INFO("CompositionRepository::save| Composition saved, id: {}, version: {}.{}",
                composition.id(), composition.version().major, composition.version().minor);
    return Error{ErrorCode::Success, ""};
  }

  Result<Composition> CompositionRepository::load(const CompositionId &id, const Version version) {
    // Query composition by ID and version
    auto query = box_composition_->query(
      ObxComposition_::compositionId.equals(id) &&
      ObxComposition_::versionMajor.equals(version.major) &&
      ObxComposition_::versionMinor.equals(version.minor)
    ).build();

    const auto obx_comp = query.findFirst();
    if (!obx_comp) {
      TF_LOG_ERROR("CompositionRepository::load| Composition not found: {} v{}.{}",
                   id, version.major, version.minor);
      return Result<Composition>(Error{ErrorCode::StorageError, "Composition not found"});
    }

    // Convert basic composition fields
    Composition composition = utils::obxCompositionToComposition(*obx_comp);
    composition.clearFragments();
    
    // Load and convert fragments
    auto frag_query = box_fragment_->query(
      ObxFragment_::compositionId.equals(obx_comp->id)
    ).order(ObxFragment_::orderIndex).build();
    auto obx_fragments = frag_query.find();

    for (const auto &obx_frag : obx_fragments) {
      auto fragment = utils::obxFragmentToFragment(obx_frag);
      composition.insertFragment(composition.fragmentCount(), std::move(fragment));
    }

    TF_LOG_INFO("CompositionRepository::load| Composition loaded: {} v{}.{}",
                id, version.major, version.minor);
    return Result(std::move(composition));
  }

  Result<Composition> CompositionRepository::loadLatest(const CompositionId &id) {
    const auto version = getLatestVersion(id);
    if (version.hasError()) {
      TF_LOG_ERROR("CompositionRepository::loadLatest| Error: {}", version.error().message);
      return Result<Composition>(version.error());
    }

    return load(id, version.value());
  }

  std::vector<CompositionId> CompositionRepository::list() {
    const auto compositions = box_composition_->getAll();

    // Extract unique composition IDs using a set to avoid duplicates
    std::unordered_set<CompositionId> unique_ids;
    for (const auto &comp: compositions) {
      unique_ids.insert(comp->compositionId);
    }

    if (unique_ids.empty()) {
      TF_LOG_WARN("CompositionRepository::list| No compositions found");
    }

    return {unique_ids.begin(), unique_ids.end()};
  }

  Result<Version> CompositionRepository::getLatestVersion(const CompositionId &id) {
    auto query = box_composition_->query(ObxComposition_::compositionId.equals(id)).build();
    auto compositions = query.find();

    if (compositions.empty()) {
      TF_LOG_ERROR("CompositionRepository::getLatestVersion| Composition not found: {}", id);
      return Result<Version>(Error{ErrorCode::StorageError, "Composition not found"});
    }

    // Find maximum version
    const auto versions = compositions | std::views::transform([](const auto &comp) {
      return Version{comp.versionMajor, comp.versionMinor};
    });

    const auto max_version = *std::ranges::max_element(versions);
    return Result{max_version};
  }

  Error CompositionRepository::deprecate(const CompositionId &id, Version version) {
    auto query = box_composition_->query(
      ObxComposition_::compositionId.equals(id) &&
      ObxComposition_::versionMajor.equals(version.major) &&
      ObxComposition_::versionMinor.equals(version.minor)
    ).build();

    const auto obx_comp = query.findFirst();
    if (!obx_comp) {
      TF_LOG_ERROR("CompositionRepository::deprecate| Composition not found: {} v{}.{}",
                   id, version.major, version.minor);
      return Error{ErrorCode::StorageError, "Composition not found"};
    }

    // Set state to Deprecated (2)
    obx_comp->state = utils::blockStateToObxStateCode(BlockState::Deprecated);

    if (!box_composition_->put(*obx_comp)) {
      TF_LOG_ERROR("CompositionRepository::deprecate| Failed to save composition");
      return Error{ErrorCode::StorageError, "Failed to save composition"};
    }

    TF_LOG_INFO("CompositionRepository::deprecate| Composition deprecated: {} v{}.{}",
                id, version.major, version.minor);
    return Error{ErrorCode::Success};
  }
} // tf
