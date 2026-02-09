//
// Created by artem.d on 28.01.2026.
//

#include "engine.h"

#include <filesystem>

#include "logger.h"
#include "objectbox-model.h"
#include "objectboxrepository.h"
#include "repository_block_cache.h"

namespace tf {
// Engine implementation

Engine::Engine() : config_{}, renderer_(std::make_unique<Renderer>()) {
  TF_LOG_INFO("Engine initialized with default config");
  full_init();
}

Engine::Engine(EngineConfig config)
    : config_(std::move(config)), renderer_(std::make_unique<Renderer>()) {
  TF_LOG_INFO("Engine initialized with custom config [project={}, db_path={}]",
              config_.project_key, config_.default_data_path);
  full_init();
}

void Engine::set_block_repository(std::shared_ptr<IBlockRepository> repo) {
  blockRepo_ = std::move(repo);
  TF_LOG_DEBUG("Block repository set");

  // Create repository-backed block cache for renderer
  if (blockRepo_) {
    renderer_->set_block_cache(
        std::make_unique<RepositoryBlockCache>(blockRepo_));
    TF_LOG_DEBUG("Renderer block cache initialized");
  }
}

void Engine::set_composition_repository(
    std::shared_ptr<ICompositionRepository> repo) {
  compRepo_ = std::move(repo);
  TF_LOG_DEBUG("Composition repository set");
}

void Engine::set_normalizer(std::shared_ptr<INormalizer> normalizer) {
  normalizer_ = std::move(normalizer);
  TF_LOG_DEBUG("Normalizer set");
}

// ==================== Block Operations ====================

Result<Block> Engine::load_block(const BlockId& id,
                                 std::optional<Version> version) const {
  if (!blockRepo_) {
    TF_LOG_ERROR("Cannot load block: no block repository configured");
    return Result<Block>(
        Error{ErrorCode::StorageError, "No block repository configured"});
  }
  if (version.has_value()) {
    TF_LOG_DEBUG("Loading block [id={}, version={}.{}]", id, version->major,
                 version->minor);
    return blockRepo_->load(id, version.value());
  }
  TF_LOG_DEBUG("Loading latest block [id={}]", id);
  return blockRepo_->load_latest(id);
}

Error Engine::deprecate_block(const BlockId& id, Version version) {
  TF_LOG_INFO("Deprecating block [id={}, version={}.{}]", id, version.major,
              version.minor);
  auto blockResult = load_block(id, version);
  if (blockResult.has_error()) {
    TF_LOG_ERROR("Failed to deprecate block [id={}]: {}", id,
                 blockResult.error().message);
    return blockResult.error();
  }

  Block block = std::move(blockResult.value());
  block.deprecate();
  auto err = save_block(block);
  if (err.is_error()) {
    TF_LOG_ERROR("Failed to save deprecated block [id={}]: {}", id,
                 err.message);
  } else {
    TF_LOG_INFO("Block deprecated successfully [id={}]", id);
  }
  return err;
}

Result<Version> Engine::get_latest_block_version(const BlockId& id) {
  if (!blockRepo_) {
    TF_LOG_ERROR(
        "Cannot get latest block version: no block repository configured");
    return Result<Version>(
        Error{ErrorCode::StorageError, "No block repository configured"});
  }
  TF_LOG_DEBUG("Getting latest block version [id={}]", id);
  return blockRepo_->get_latest_version(id);
}

std::vector<BlockId> Engine::list_blocks(std::optional<BlockType> typeFilter) {
  if (!blockRepo_) {
    TF_LOG_WARN("Cannot list blocks: no block repository configured");
    return {};
  }
  TF_LOG_DEBUG("Listing blocks");
  return blockRepo_->list(typeFilter);
}

Result<Version> Engine::get_next_version(const BlockId& id, VersionBump bump) {
  auto current = blockRepo_->get_latest_version(id);

  Version next{1, 0};
  if (current.has_value()) {
    if (bump == VersionBump::Major) {
      next = Version{static_cast<uint16_t>(current.value().major + 1), 0};
    } else {
      // Minor
      next = Version{current.value().major,
                     static_cast<uint16_t>(current.value().minor + 1)};
    }
  }

  return Result<Version>(next);
}

Result<PublishedBlock> Engine::publish_block(BlockDraft draft,
                                             VersionBump bump) {
  Block& block = draft.internal_;

  if (block.state() != BlockState::Draft) {
    return Result<PublishedBlock>(Error{ErrorCode::InvalidStateTransition,
                                        "Only Draft blocks can be published"});
  }

  auto next_ver_result = get_next_version(block.id(), bump);
  if (next_ver_result.has_error()) {
    return Result<PublishedBlock>(next_ver_result.error());
  }

  return publish_block_internal(std::move(block), next_ver_result.value());
}

Result<PublishedBlock> Engine::publish_block(BlockDraft draft,
                                             Version explicit_version) {
  Block& block = draft.internal_;

  if (block.state() != BlockState::Draft) {
    return Result<PublishedBlock>(Error{ErrorCode::InvalidStateTransition,
                                        "Only Draft blocks can be published"});
  }

  auto existing = blockRepo_->load(block.id(), explicit_version);
  if (existing.has_value()) {
    return Result<PublishedBlock>(
        Error{ErrorCode::DuplicateId,
              std::format("Version {}.{} already exists",
                          explicit_version.major, explicit_version.minor)});
  }

  return publish_block_internal(std::move(block), explicit_version);
}

Result<PublishedBlock> Engine::publish_block_internal(Block block,
                                                      Version version) {
  auto err = block.publish(version);
  if (err.is_error()) {
    return Result<PublishedBlock>(err);
  }

  err = blockRepo_->save(block);
  if (err.is_error()) {
    return Result<PublishedBlock>(err);
  }

  TF_LOG_INFO("Block published: {} v{}", block.id(), version);

  return Result<PublishedBlock>(PublishedBlock(block.id(), version));
}

Result<Version> Engine::get_latest_composition_version(
    const CompositionId& id) {
  if (!compRepo_) {
    TF_LOG_ERROR(
        "Cannot get latest composition version: no composition repository "
        "configured");
    return Result<Version>(
        Error{ErrorCode::StorageError, "No composition repository configured"});
  }

  TF_LOG_DEBUG("Getting latest composition version [id={}]", id);
  const auto version = compRepo_->get_latest_version(id);
  return version;
}

Result<PublishedComposition> Engine::publish_composition(CompositionDraft draft,
                                                         VersionBump bump) {
  Composition& comp = draft.internal_;

  if (comp.state() != BlockState::Draft) {
    return Result<PublishedComposition>(
        Error{ErrorCode::InvalidStateTransition,
              "Only Draft compositions can be published"});
  }

  for (const auto& frag : comp.fragments()) {
    if (frag.is_block_ref()) {
      const auto& ref = frag.as_block_ref();
      if (ref.use_latest()) {
        return Result<PublishedComposition>(Error::version_required());
      }
    }
  }

  Version next_version{1, 0};
  auto latest = compRepo_->get_latest_version(comp.id());
  if (latest.has_value()) {
    if (bump == VersionBump::Major) {
      next_version =
          Version{static_cast<uint16_t>(latest.value().major + 1), 0};
    } else {
      next_version = Version{latest.value().major,
                             static_cast<uint16_t>(latest.value().minor + 1)};
    }
  }

  return publish_composition_internal(comp, next_version);
}

Result<PublishedComposition> Engine::publish_composition(
    CompositionDraft draft, Version explicit_version) {
  Composition& comp = draft.internal_;

  if (comp.state() != BlockState::Draft) {
    return Result<PublishedComposition>(
        Error{ErrorCode::InvalidStateTransition,
              "Only Draft compositions can be published"});
  }

  if (auto existing = compRepo_->load(comp.id(), explicit_version);
      existing.has_value()) {
    return Result<PublishedComposition>(
        Error{ErrorCode::DuplicateId, "Composition version already exists"});
  }

  for (const auto& frag : comp.fragments()) {
    if (frag.is_block_ref() && frag.as_block_ref().use_latest()) {
      return Result<PublishedComposition>(Error::version_required());
    }
  }

  return publish_composition_internal(std::move(comp), explicit_version);
}

Result<PublishedComposition> Engine::publish_composition_internal(
    Composition comp, Version version) {
  auto err = comp.publish(version);
  if (err.is_error()) return Result<PublishedComposition>(err);

  err = compRepo_->save(comp);
  if (err.is_error()) return Result<PublishedComposition>(err);

  return Result<PublishedComposition>(PublishedComposition(comp.id(), version));
}

Error Engine::save_block(const Block& block) {
  if (!blockRepo_) {
    TF_LOG_ERROR("Cannot save block: no block repository configured");
    return Error{ErrorCode::StorageError, "No block repository configured"};
  }
  TF_LOG_INFO("Saving block [id={}, version={}.{}]", block.id(),
              block.version().major, block.version().minor);
  return blockRepo_->save(block);
}

// ==================== Composition Operations ====================

Error Engine::save_composition(const Composition& composition) {
  if (!compRepo_) {
    TF_LOG_ERROR(
        "Cannot save composition: no composition repository configured");
    return Error{ErrorCode::StorageError,
                 "No composition repository configured"};
  }
  TF_LOG_INFO("Saving composition [id={}, version={}.{}]", composition.id(),
              composition.version().major, composition.version().minor);
  return compRepo_->save(composition);
}

Result<Composition> Engine::load_composition(const CompositionId& id,
                                             std::optional<Version> version) {
  if (!compRepo_) {
    TF_LOG_ERROR(
        "Cannot load composition: no composition repository configured");
    return Result<Composition>(
        Error{ErrorCode::StorageError, "No composition repository configured"});
  }
  if (version.has_value()) {
    TF_LOG_DEBUG("Loading composition [id={}, version={}.{}]", id,
                 version->major, version->minor);
    return compRepo_->load(id, version.value());
  }
  TF_LOG_DEBUG("Loading latest composition [id={}]", id);
  return compRepo_->load_latest(id);
}

Error Engine::deprecate_composition(const CompositionId& id, Version version) {
  TF_LOG_INFO("Deprecating composition [id={}, version={}.{}]", id,
              version.major, version.minor);
  auto compResult = load_composition(id, version);
  if (compResult.has_error()) {
    TF_LOG_ERROR("Failed to deprecate composition [id={}]: {}", id,
                 compResult.error().message);
    return compResult.error();
  }

  Composition comp = std::move(compResult.value());
  comp.deprecate();
  auto err = save_composition(comp);
  if (err.is_error()) {
    TF_LOG_ERROR("Failed to save deprecated composition [id={}]: {}", id,
                 err.message);
  } else {
    TF_LOG_INFO("Composition deprecated successfully [id={}]", id);
  }
  return err;
}

std::vector<CompositionId> Engine::list_compositions() const {
  if (!compRepo_) {
    TF_LOG_WARN(
        "Cannot list compositions: no composition repository configured");
    return {};
  }
  TF_LOG_DEBUG("Listing compositions");
  return compRepo_->list();
}

// ==================== Rendering Operations ====================

Result<RenderResult> Engine::render(const CompositionId& compositionId,
                                    const RenderContext& context) {
  TF_LOG_INFO("Rendering composition [id={}]", compositionId);

  // Clear cache before rendering to ensure fresh data and prevent memory growth
  renderer_->clear_cache();

  auto compResult = load_composition(compositionId);
  if (compResult.has_error()) {
    TF_LOG_ERROR("Failed to render composition [id={}]: {}", compositionId,
                 compResult.error().message);
    return Result<RenderResult>(compResult.error());
  }
  return renderer_->render(compResult.value(), context);
}

Result<RenderResult> Engine::render(const CompositionId& compositionId,
                                    Version version,
                                    const RenderContext& context) {
  TF_LOG_INFO("Rendering composition [id={}, version={}.{}]", compositionId,
              version.major, version.minor);

  // Clear cache before rendering to ensure fresh data and prevent memory growth
  renderer_->clear_cache();

  auto compResult = load_composition(compositionId, version);
  if (compResult.has_error()) {
    TF_LOG_ERROR("Failed to render composition [id={}, version={}.{}]: {}",
                 compositionId, version.major, version.minor,
                 compResult.error().message);
    return Result<RenderResult>(compResult.error());
  }
  return renderer_->render(compResult.value(), context);
}

Result<std::string> Engine::render_block(const BlockId& blockId,
                                         const RenderContext& context) {
  TF_LOG_INFO("Rendering block [id={}]", blockId);

  // Clear cache before rendering to ensure fresh data and prevent memory growth
  renderer_->clear_cache();

  auto blockResult = load_block(blockId);
  if (blockResult.has_error()) {
    TF_LOG_ERROR("Failed to render block [id={}]: {}", blockId,
                 blockResult.error().message);
    return Result<std::string>(blockResult.error());
  }
  return renderer_->render_block(blockResult.value(), context);
}

Result<std::string> Engine::render_block(const BlockId& blockId,
                                         Version version,
                                         const RenderContext& context) {
  TF_LOG_INFO("Rendering block [id={}, version={}.{}]", blockId, version.major,
              version.minor);

  // Clear cache before rendering to ensure fresh data and prevent memory growth
  renderer_->clear_cache();

  auto blockResult = load_block(blockId, version);
  if (blockResult.has_error()) {
    TF_LOG_ERROR("Failed to render block [id={}, version={}.{}]: {}", blockId,
                 version.major, version.minor, blockResult.error().message);
    return Result<std::string>(blockResult.error());
  }
  return renderer_->render_block(blockResult.value(), context);
}

// ==================== Normalization ====================

Result<std::string> Engine::normalize(const std::string& text,
                                      const SemanticStyle& style) const {
  TF_LOG_DEBUG("Normalizing text (length={})", text.length());
  if (!normalizer_) {
    TF_LOG_ERROR("Cannot normalize: no normalizer configured");
    return Result<std::string>(
        Error{ErrorCode::StorageError, "No normalizer configured"});
  }
  return normalizer_->normalize(text, style);
}

bool Engine::hasNormalizer() const noexcept { return normalizer_ != nullptr; }

// ==================== Validation ====================

Error Engine::validate_composition(const CompositionId& id) {
  TF_LOG_DEBUG("Validating composition [id={}]", id);
  auto compResult = load_composition(id);
  if (compResult.has_error()) {
    TF_LOG_ERROR("Failed to validate composition [id={}]: {}", id,
                 compResult.error().message);
    return compResult.error();
  }
  auto err = compResult.value().validate();
  if (err.is_error()) {
    TF_LOG_ERROR("Composition validation failed [id={}]: {}", id, err.message);
  } else {
    TF_LOG_DEBUG("Composition validated successfully [id={}]", id);
  }
  return err;
}

Error Engine::validate_block(const BlockId& id) {
  TF_LOG_DEBUG("Validating block [id={}]", id);
  auto blockResult = load_block(id);
  if (blockResult.has_error()) {
    TF_LOG_ERROR("Failed to validate block [id={}]: {}", id,
                 blockResult.error().message);
    return blockResult.error();
  }
  auto block = blockResult.value();
  auto err = block.validate_params({}, {});
  if (err.is_error()) {
    TF_LOG_ERROR("Block validation failed [id={}]: {}", id, err.message);
  } else {
    TF_LOG_DEBUG("Block validated successfully [id={}]", id);
  }
  return err;
}

void Engine::full_init() {
  // Ensure the database directory exists
  const bool is_in_memory = config_.default_data_path.starts_with("memory:");
  if (!is_in_memory && !std::filesystem::exists(config_.default_data_path)) {
    std::filesystem::create_directories(config_.default_data_path);
  }

  obx::Options options;
  options.directory(config_.default_data_path);
  options.model(create_obx_model());

  auto store = std::make_shared<obx::Store>(options);
  const std::shared_ptr<IBlockRepository> block_repository =
      std::make_shared<BlockRepository>(store);
  const std::shared_ptr<ICompositionRepository> composition_repository =
      std::make_shared<CompositionRepository>(store);

  set_block_repository(block_repository);
  set_composition_repository(composition_repository);
}
}  // namespace tf
