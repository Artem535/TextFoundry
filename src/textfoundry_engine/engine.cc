//
// Created by artem.d on 28.01.2026.
//

#include "engine.h"

#include <unordered_map>

namespace tf {

// Hash function for pair<BlockId, Version>
struct PairHash {
    size_t operator()(const std::pair<BlockId, Version>& p) const {
        return std::hash<BlockId>{}(p.first) ^
               (std::hash<uint16_t>{}(p.second.major) << 1) ^
               (std::hash<uint16_t>{}(p.second.minor) << 2);
    }
};

// In-memory block cache for renderer (stub implementation)
class InMemoryBlockCache : public IBlockCache {
public:
    void addBlock(const Block& block) {
        auto key = std::make_pair(block.id(), block.version());
        blocks_[key] = block;
    }

    const Block* getBlock(const BlockId& id, Version version) const override {
        auto key = std::make_pair(id, version);
        auto it = blocks_.find(key);
        if (it != blocks_.end()) {
            return &it->second;
        }
        return nullptr;
    }

    const Block* getLatestBlock(const BlockId& id) const override {
        const Block* latest = nullptr;
        Version latestVer{0, 0};

        for (const auto& [key, block] : blocks_) {
            if (key.first == id && block.state() == BlockState::Published) {
                if (!latest || block.version() > latestVer) {
                    latest = &block;
                    latestVer = block.version();
                }
            }
        }
        return latest;
    }

private:
    mutable std::unordered_map<std::pair<BlockId, Version>, Block, PairHash> blocks_;
};

// Engine implementation

Engine::Engine() : config_{}, renderer_(std::make_unique<Renderer>()) {}

Engine::Engine(EngineConfig config)
    : config_(std::move(config)),
      renderer_(std::make_unique<Renderer>()) {}

void Engine::setBlockRepository(std::shared_ptr<IBlockRepository> repo) {
    blockRepo_ = std::move(repo);
    initRenderer();
}

void Engine::setCompositionRepository(std::shared_ptr<ICompositionRepository> repo) {
    compRepo_ = std::move(repo);
}

void Engine::setNormalizer(std::shared_ptr<INormalizer> normalizer) {
    normalizer_ = std::move(normalizer);
}

void Engine::initRenderer() {
    // TODO: Create proper block cache adapter
    // renderer_->setBlockCache(...);
}

// ==================== Block Operations ====================

Block Engine::createBlockDraft(const BlockId& id) {
    return Block(id);
}

Error Engine::saveBlock(const Block& block) {
    if (!blockRepo_) {
        return Error{ErrorCode::StorageError, "No block repository configured"};
    }
    return blockRepo_->save(block);
}

Result<Block> Engine::loadBlock(const BlockId& id, std::optional<Version> version) {
    if (!blockRepo_) {
        return Error{ErrorCode::StorageError, "No block repository configured"};
    }
    if (version.has_value()) {
        return blockRepo_->load(id, version.value());
    }
    return blockRepo_->loadLatest(id);
}

Result<Block> Engine::publishBlock(const BlockId& id, Version newVersion) {
    auto blockResult = loadBlock(id);
    if (blockResult.hasError()) {
        return blockResult.error();
    }

    Block block = std::move(blockResult.value());
    auto err = block.publish(newVersion);
    if (err.isError()) {
        return err;
    }

    err = saveBlock(block);
    if (err.isError()) {
        return err;
    }

    return block;
}

Error Engine::deprecateBlock(const BlockId& id, Version version) {
    auto blockResult = loadBlock(id, version);
    if (blockResult.hasError()) {
        return blockResult.error();
    }

    Block block = std::move(blockResult.value());
    block.deprecate();
    return saveBlock(block);
}

Result<Version> Engine::getLatestBlockVersion(const BlockId& id) {
    if (!blockRepo_) {
        return Error{ErrorCode::StorageError, "No block repository configured"};
    }
    return blockRepo_->getLatestVersion(id);
}

std::vector<BlockId> Engine::listBlocks(std::optional<BlockType> typeFilter) {
    if (!blockRepo_) {
        return {};
    }
    return blockRepo_->list(typeFilter);
}

// ==================== Composition Operations ====================

Composition Engine::createCompositionDraft(const CompositionId& id) {
    return Composition(id);
}

Error Engine::saveComposition(const Composition& composition) {
    if (!compRepo_) {
        return Error{ErrorCode::StorageError, "No composition repository configured"};
    }
    return compRepo_->save(composition);
}

Result<Composition> Engine::loadComposition(
    const CompositionId& id,
    std::optional<Version> version
) {
    if (!compRepo_) {
        return Error{ErrorCode::StorageError, "No composition repository configured"};
    }
    if (version.has_value()) {
        return compRepo_->load(id, version.value());
    }
    return compRepo_->loadLatest(id);
}

Result<Composition> Engine::publishComposition(const CompositionId& id, Version newVersion) {
    auto compResult = loadComposition(id);
    if (compResult.hasError()) {
        return compResult.error();
    }

    Composition comp = std::move(compResult.value());
    auto err = comp.publish(newVersion);
    if (err.isError()) {
        return err;
    }

    err = saveComposition(comp);
    if (err.isError()) {
        return err;
    }

    return comp;
}

Error Engine::deprecateComposition(const CompositionId& id, Version version) {
    auto compResult = loadComposition(id, version);
    if (compResult.hasError()) {
        return compResult.error();
    }

    Composition comp = std::move(compResult.value());
    comp.deprecate();
    return saveComposition(comp);
}

std::vector<CompositionId> Engine::listCompositions() {
    if (!compRepo_) {
        return {};
    }
    return compRepo_->list();
}

// ==================== Rendering Operations ====================

Result<RenderResult> Engine::render(
    const CompositionId& compositionId,
    const RenderContext& context
) {
    auto compResult = loadComposition(compositionId);
    if (compResult.hasError()) {
        return compResult.error();
    }
    return renderer_->render(compResult.value(), context);
}

Result<RenderResult> Engine::render(
    const CompositionId& compositionId,
    Version version,
    const RenderContext& context
) {
    auto compResult = loadComposition(compositionId, version);
    if (compResult.hasError()) {
        return compResult.error();
    }
    return renderer_->render(compResult.value(), context);
}

Result<std::string> Engine::renderBlock(
    const BlockId& blockId,
    const RenderContext& context
) {
    auto blockResult = loadBlock(blockId);
    if (blockResult.hasError()) {
        return blockResult.error();
    }
    return renderer_->renderBlock(blockResult.value(), context);
}

Result<std::string> Engine::renderBlock(
    const BlockId& blockId,
    Version version,
    const RenderContext& context
) {
    auto blockResult = loadBlock(blockId, version);
    if (blockResult.hasError()) {
        return blockResult.error();
    }
    return renderer_->renderBlock(blockResult.value(), context);
}

// ==================== Normalization ====================

Result<std::string> Engine::normalize(const std::string& text, const SemanticStyle& style) const {
    if (!normalizer_) {
        return Error{ErrorCode::StorageError, "No normalizer configured"};
    }
    return normalizer_->normalize(text, style);
}

bool Engine::hasNormalizer() const noexcept {
    return normalizer_ != nullptr;
}

// ==================== Validation ====================

Error Engine::validateComposition(const CompositionId& id) {
    auto compResult = loadComposition(id);
    if (compResult.hasError()) {
        return compResult.error();
    }
    return compResult.value().validate();
}

Error Engine::validateBlock(const BlockId& id) {
    auto blockResult = loadBlock(id);
    if (blockResult.hasError()) {
        return blockResult.error();
    }
    auto block = blockResult.value();
    return block.validateParams({}, {});
}

// Stub implementations for repository getters
const Block* Engine::getBlockFromRepo(const BlockId& id, Version version) const {
    if (!blockRepo_) {
        return nullptr;
    }
    auto result = blockRepo_->load(id, version);
    if (result.hasError()) {
        return nullptr;
    }
    // This is a stub - in real implementation we'd need proper caching
    return nullptr;
}

const Block* Engine::getLatestBlockFromRepo(const BlockId& id) const {
    if (!blockRepo_) {
        return nullptr;
    }
    auto result = blockRepo_->loadLatest(id);
    if (result.hasError()) {
        return nullptr;
    }
    // This is a stub - in real implementation we'd need proper caching
    return nullptr;
}

} // namespace tf
