//
// Created by artem.d on 30.01.2026.
//

#include "objectboxstorage.h"

#include "obx_utils.hpp"
#include "logger.h"

namespace tf {
  BlockRepository::BlockRepository(std::shared_ptr<obx::Store> store) : store_(std::move(store)) {
    project_box_ = std::make_unique<obx::Box<ObxProject> >(*store_);
    block_box_ = std::make_unique<obx::Box<ObxBlock> >(*store_);
  }

  Error BlockRepository::save(const Block &block) {
  }

  Result<Block> BlockRepository::load(const BlockId &id, const Version version) {
    auto query = block_box_->query(
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
  }

  std::vector<BlockId> BlockRepository::list(std::optional<BlockType> typeFilter) {
  }

  Result<Version> BlockRepository::getLatestVersion(const BlockId &id) {
  }

  Error BlockRepository::deprecate(const BlockId &id, Version version) {
  }
} // tf
