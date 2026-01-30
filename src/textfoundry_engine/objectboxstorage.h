//
// Created by artem.d on 30.01.2026.
//

#pragma once

#include "engine.h"

#define OBX_CPP_FILE
#include <objectbox.hpp>
#include "database_scheme.obx.hpp"
#include "objectbox-model.h"

#include <memory>


namespace tf {
  class BlockRepository final : public IBlockRepository {
  public:
    explicit BlockRepository(std::shared_ptr<obx::Store> store);

    [[nodiscard]] Error save(const Block &block) override;

    [[nodiscard]] Result<Block> load(const BlockId &id, Version version) override;

    [[nodiscard]] Result<Block> loadLatest(const BlockId &id) override;

    [[nodiscard]] std::vector<BlockId> list(std::optional<BlockType> typeFilter) override;

    [[nodiscard]] Result<Version> getLatestVersion(const BlockId &id) override;

    [[nodiscard]] Error deprecate(const BlockId &id, Version version) override;

  private:
    std::shared_ptr< obx::Store> store_;
    std::unique_ptr<obx::Box<ObxProject>> project_box_;
    std::unique_ptr<obx::Box<ObxBlock>> block_box_;
  };

} // tf
