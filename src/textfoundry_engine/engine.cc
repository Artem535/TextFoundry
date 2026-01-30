//
// Created by artem.d on 28.01.2026.
//

#include "engine.h"
#include "logger.h"

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

Engine::Engine() : config_{}, renderer_(std::make_unique<Renderer>()) {
    TF_LOG_INFO("Engine initialized with default config");
}

Engine::Engine(EngineConfig config)
    : config_(std::move(config)),
      renderer_(std::make_unique<Renderer>()) {
    TF_LOG_INFO("Engine initialized with custom config [project={}]", config_.projectKey);
}

void Engine::setBlockRepository(std::shared_ptr<IBlockRepository> repo) {
    blockRepo_ = std::move(repo);
    TF_LOG_DEBUG("Block repository set");
    initRenderer();
}

void Engine::setCompositionRepository(std::shared_ptr<ICompositionRepository> repo) {
    compRepo_ = std::move(repo);
    TF_LOG_DEBUG("Composition repository set");
}

void Engine::setNormalizer(std::shared_ptr<INormalizer> normalizer) {
    normalizer_ = std::move(normalizer);
    TF_LOG_DEBUG("Normalizer set");
}

void Engine::initRenderer() {
    // TODO: Create proper block cache adapter
    // renderer_->setBlockCache(...);
    TF_LOG_DEBUG("Renderer initialized");
}

// ==================== Block Operations ====================

Block Engine::createBlockDraft(const BlockId& id) {
    TF_LOG_DEBUG("Creating block draft [id={}]", id);
    return Block(id);
}

Error Engine::saveBlock(const Block& block) {
    if (!blockRepo_) {
        TF_LOG_ERROR("Cannot save block: no block repository configured");
        return Error{ErrorCode::StorageError, "No block repository configured"};
    }
    TF_LOG_INFO("Saving block [id={}, version={}.{}]", block.id(), block.version().major, block.version().minor);
    return blockRepo_->save(block);
}

Result<Block> Engine::loadBlock(const BlockId& id, std::optional<Version> version) {
    if (!blockRepo_) {
        TF_LOG_ERROR("Cannot load block: no block repository configured");
        return Result<Block>(Error{ErrorCode::StorageError, "No block repository configured"});
    }
    if (version.has_value()) {
        TF_LOG_DEBUG("Loading block [id={}, version={}.{}]", id, version->major, version->minor);
        return blockRepo_->load(id, version.value());
    }
    TF_LOG_DEBUG("Loading latest block [id={}]", id);
    return blockRepo_->loadLatest(id);
}

Result<Block> Engine::publishBlock(const BlockId& id, Version newVersion) {
    TF_LOG_INFO("Publishing block [id={} -> version={}.{}]", id, newVersion.major, newVersion.minor);
    auto blockResult = loadBlock(id);
    if (blockResult.hasError()) {
        TF_LOG_ERROR("Failed to publish block [id={}]: {}", id, blockResult.error().message);
        return Result<Block>(blockResult.error());
    }

    Block block = std::move(blockResult.value());
    auto err = block.publish(newVersion);
    if (err.isError()) {
        TF_LOG_ERROR("Failed to publish block [id={}]: {}", id, err.message);
        return Result<Block>(err);
    }

    err = saveBlock(block);
    if (err.isError()) {
        TF_LOG_ERROR("Failed to save published block [id={}]: {}", id, err.message);
        return Result<Block>(err);
    }

    TF_LOG_INFO("Block published successfully [id={}, version={}.{}]", id, newVersion.major, newVersion.minor);
    return Result<Block>(block);
}

Error Engine::deprecateBlock(const BlockId& id, Version version) {
    TF_LOG_INFO("Deprecating block [id={}, version={}.{}]", id, version.major, version.minor);
    auto blockResult = loadBlock(id, version);
    if (blockResult.hasError()) {
        TF_LOG_ERROR("Failed to deprecate block [id={}]: {}", id, blockResult.error().message);
        return blockResult.error();
    }

    Block block = std::move(blockResult.value());
    block.deprecate();
    auto err = saveBlock(block);
    if (err.isError()) {
        TF_LOG_ERROR("Failed to save deprecated block [id={}]: {}", id, err.message);
    } else {
        TF_LOG_INFO("Block deprecated successfully [id={}]", id);
    }
    return err;
}

Result<Version> Engine::getLatestBlockVersion(const BlockId& id) {
    if (!blockRepo_) {
        TF_LOG_ERROR("Cannot get latest block version: no block repository configured");
        return Result<Version>(Error{ErrorCode::StorageError, "No block repository configured"});
    }
    TF_LOG_DEBUG("Getting latest block version [id={}]", id);
    return blockRepo_->getLatestVersion(id);
}

std::vector<BlockId> Engine::listBlocks(std::optional<BlockType> typeFilter) {
    if (!blockRepo_) {
        TF_LOG_WARN("Cannot list blocks: no block repository configured");
        return {};
    }
    TF_LOG_DEBUG("Listing blocks");
    return blockRepo_->list(typeFilter);
}

// ==================== Composition Operations ====================

Composition Engine::createCompositionDraft(const CompositionId& id) {
    TF_LOG_DEBUG("Creating composition draft [id={}]", id);
    return Composition(id);
}

Error Engine::saveComposition(const Composition& composition) {
    if (!compRepo_) {
        TF_LOG_ERROR("Cannot save composition: no composition repository configured");
        return Error{ErrorCode::StorageError, "No composition repository configured"};
    }
    TF_LOG_INFO("Saving composition [id={}, version={}.{}]", composition.id(), composition.version().major, composition.version().minor);
    return compRepo_->save(composition);
}

Result<Composition> Engine::loadComposition(
    const CompositionId& id,
    std::optional<Version> version
) {
    if (!compRepo_) {
        TF_LOG_ERROR("Cannot load composition: no composition repository configured");
        return Result<Composition>(Error{ErrorCode::StorageError, "No composition repository configured"});
    }
    if (version.has_value()) {
        TF_LOG_DEBUG("Loading composition [id={}, version={}.{}]", id, version->major, version->minor);
        return compRepo_->load(id, version.value());
    }
    TF_LOG_DEBUG("Loading latest composition [id={}]", id);
    return compRepo_->loadLatest(id);
}

Result<Composition> Engine::publishComposition(const CompositionId& id, Version newVersion) {
    TF_LOG_INFO("Publishing composition [id={} -> version={}.{}]", id, newVersion.major, newVersion.minor);
    auto compResult = loadComposition(id);
    if (compResult.hasError()) {
        TF_LOG_ERROR("Failed to publish composition [id={}]: {}", id, compResult.error().message);
        return Result<Composition>(compResult.error());
    }

    Composition comp = std::move(compResult.value());
    auto err = comp.publish(newVersion);
    if (err.isError()) {
        TF_LOG_ERROR("Failed to publish composition [id={}]: {}", id, err.message);
        return Result<Composition>(err);
    }

    err = saveComposition(comp);
    if (err.isError()) {
        TF_LOG_ERROR("Failed to save published composition [id={}]: {}", id, err.message);
        return Result<Composition>(err);
    }

    TF_LOG_INFO("Composition published successfully [id={}, version={}.{}]", id, newVersion.major, newVersion.minor);
    return Result<Composition>(comp);
}

Error Engine::deprecateComposition(const CompositionId& id, Version version) {
    TF_LOG_INFO("Deprecating composition [id={}, version={}.{}]", id, version.major, version.minor);
    auto compResult = loadComposition(id, version);
    if (compResult.hasError()) {
        TF_LOG_ERROR("Failed to deprecate composition [id={}]: {}", id, compResult.error().message);
        return compResult.error();
    }

    Composition comp = std::move(compResult.value());
    comp.deprecate();
    auto err = saveComposition(comp);
    if (err.isError()) {
        TF_LOG_ERROR("Failed to save deprecated composition [id={}]: {}", id, err.message);
    } else {
        TF_LOG_INFO("Composition deprecated successfully [id={}]", id);
    }
    return err;
}

std::vector<CompositionId> Engine::listCompositions() {
    if (!compRepo_) {
        TF_LOG_WARN("Cannot list compositions: no composition repository configured");
        return {};
    }
    TF_LOG_DEBUG("Listing compositions");
    return compRepo_->list();
}

// ==================== Rendering Operations ====================

Result<RenderResult> Engine::render(
    const CompositionId& compositionId,
    const RenderContext& context
) {
    TF_LOG_INFO("Rendering composition [id={}]", compositionId);
    auto compResult = loadComposition(compositionId);
    if (compResult.hasError()) {
        TF_LOG_ERROR("Failed to render composition [id={}]: {}", compositionId, compResult.error().message);
        return Result<RenderResult>(compResult.error());
    }
    return renderer_->render(compResult.value(), context);
}

Result<RenderResult> Engine::render(
    const CompositionId& compositionId,
    Version version,
    const RenderContext& context
) {
    TF_LOG_INFO("Rendering composition [id={}, version={}.{}]", compositionId, version.major, version.minor);
    auto compResult = loadComposition(compositionId, version);
    if (compResult.hasError()) {
        TF_LOG_ERROR("Failed to render composition [id={}, version={}.{}]: {}", 
                     compositionId, version.major, version.minor, compResult.error().message);
        return Result<RenderResult>(compResult.error());
    }
    return renderer_->render(compResult.value(), context);
}

Result<std::string> Engine::renderBlock(
    const BlockId& blockId,
    const RenderContext& context
) {
    TF_LOG_INFO("Rendering block [id={}]", blockId);
    auto blockResult = loadBlock(blockId);
    if (blockResult.hasError()) {
        TF_LOG_ERROR("Failed to render block [id={}]: {}", blockId, blockResult.error().message);
        return Result<std::string>(blockResult.error());
    }
    return renderer_->renderBlock(blockResult.value(), context);
}

Result<std::string> Engine::renderBlock(
    const BlockId& blockId,
    Version version,
    const RenderContext& context
) {
    TF_LOG_INFO("Rendering block [id={}, version={}.{}]", blockId, version.major, version.minor);
    auto blockResult = loadBlock(blockId, version);
    if (blockResult.hasError()) {
        TF_LOG_ERROR("Failed to render block [id={}, version={}.{}]: {}", 
                     blockId, version.major, version.minor, blockResult.error().message);
        return Result<std::string>(blockResult.error());
    }
    return renderer_->renderBlock(blockResult.value(), context);
}

// ==================== Normalization ====================

Result<std::string> Engine::normalize(const std::string& text, const SemanticStyle& style) const {
    TF_LOG_DEBUG("Normalizing text (length={})", text.length());
    if (!normalizer_) {
        TF_LOG_ERROR("Cannot normalize: no normalizer configured");
        return Result<std::string>(Error{ErrorCode::StorageError, "No normalizer configured"});
    }
    return normalizer_->normalize(text, style);
}

bool Engine::hasNormalizer() const noexcept {
    return normalizer_ != nullptr;
}

// ==================== Validation ====================

Error Engine::validateComposition(const CompositionId& id) {
    TF_LOG_DEBUG("Validating composition [id={}]", id);
    auto compResult = loadComposition(id);
    if (compResult.hasError()) {
        TF_LOG_ERROR("Failed to validate composition [id={}]: {}", id, compResult.error().message);
        return compResult.error();
    }
    auto err = compResult.value().validate();
    if (err.isError()) {
        TF_LOG_ERROR("Composition validation failed [id={}]: {}", id, err.message);
    } else {
        TF_LOG_DEBUG("Composition validated successfully [id={}]", id);
    }
    return err;
}

Error Engine::validateBlock(const BlockId& id) {
    TF_LOG_DEBUG("Validating block [id={}]", id);
    auto blockResult = loadBlock(id);
    if (blockResult.hasError()) {
        TF_LOG_ERROR("Failed to validate block [id={}]: {}", id, blockResult.error().message);
        return blockResult.error();
    }
    auto block = blockResult.value();
    auto err = block.validateParams({}, {});
    if (err.isError()) {
        TF_LOG_ERROR("Block validation failed [id={}]: {}", id, err.message);
    } else {
        TF_LOG_DEBUG("Block validated successfully [id={}]", id);
    }
    return err;
}

// Stub implementations for repository getters
const Block* Engine::getBlockFromRepo(const BlockId& id, Version version) const {
    if (!blockRepo_) {
        TF_LOG_WARN("Cannot get block from repo: no block repository configured");
        return nullptr;
    }
    auto result = blockRepo_->load(id, version);
    if (result.hasError()) {
        TF_LOG_ERROR("Failed to get block from repo [id={}, version={}.{}]: {}", 
                     id, version.major, version.minor, result.error().message);
        return nullptr;
    }
    // This is a stub - in real implementation we'd need proper caching
    return nullptr;
}

const Block* Engine::getLatestBlockFromRepo(const BlockId& id) const {
    if (!blockRepo_) {
        TF_LOG_WARN("Cannot get latest block from repo: no block repository configured");
        return nullptr;
    }
    auto result = blockRepo_->loadLatest(id);
    if (result.hasError()) {
        TF_LOG_ERROR("Failed to get latest block from repo [id={}]: {}", id, result.error().message);
        return nullptr;
    }
    // This is a stub - in real implementation we'd need proper caching
    return nullptr;
}

} // namespace tf
