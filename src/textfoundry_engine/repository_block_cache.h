//
// Created by artem.d on 31.01.2026.
//

#pragma once

#include "renderer.h"
#include "engine.h"

#include <memory>
#include <unordered_map>
#include <utility>

namespace tf {

// Hash function for pair<BlockId, Version>
struct BlockVersionHash {
    size_t operator()(const std::pair<BlockId, Version>& p) const {
        return std::hash<BlockId>{}(p.first) ^
               (std::hash<uint16_t>{}(p.second.major) << 1) ^
               (std::hash<uint16_t>{}(p.second.minor) << 2);
    }
};

/**
 * Repository-backed block cache with lazy loading
 *
 * Implements IBlockCache interface using IBlockRepository as the underlying storage.
 * Caches blocks in memory for fast repeated access during rendering.
 * Must be cleared before each render operation to prevent memory growth.
 */
class RepositoryBlockCache final : public IBlockCache {
public:
    /**
     * Constructs cache backed by the given repository
     * @param repo Block repository for loading blocks on cache miss
     */
    explicit RepositoryBlockCache(std::shared_ptr<IBlockRepository> repo)
        : repo_(std::move(repo)) {}

    /**
     * Get block by ID and version (with caching)
     * @returns Block pointer or nullptr if not found in repository
     */
    [[nodiscard]] const Block* get_block(const BlockId& id, Version version) const override;

    /**
     * Get latest published version of a block (with caching)
     * @returns Block pointer or nullptr if not found
     */
    [[nodiscard]] const Block* get_latest_block(const BlockId& id) const override;

    /**
     * Clear all cached blocks
     * Must be called before each render operation to prevent memory growth
     */
    void clear() override;

private:
    std::shared_ptr<IBlockRepository> repo_;
    mutable std::unordered_map<std::pair<BlockId, Version>, Block, BlockVersionHash> cache_;
    mutable std::unordered_map<BlockId, Block> latestCache_;
};

} // namespace tf
