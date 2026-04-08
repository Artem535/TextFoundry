//
// Created by artem.d on 28.01.2026.
//

#include "engine.h"

#include <filesystem>
#include <regex>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "logger.h"
#include "objectbox-model.h"
#include "objectbox_repository.h"
#include "repository_block_cache.h"

namespace tf {
namespace {

std::unordered_set<std::string> DeduplicateTags(
    const std::vector<std::string>& tags) {
  return std::unordered_set<std::string>(tags.begin(), tags.end());
}

bool IsValidGeneratedBlockId(const std::string& id) {
  static const std::regex kPattern(
      R"(^[a-z0-9][a-z0-9_]*(\.[a-z0-9][a-z0-9_]*)*$)");
  return std::regex_match(id, kPattern);
}

Result<GeneratedBlockData> ValidateGeneratedBlockData(
    GeneratedBlockData data, const std::vector<BlockId>& existing_block_ids,
    IBlockRepository* block_repo, bool allow_id_collision) {
  if (data.id.empty()) {
    return Result<GeneratedBlockData>(
        Error{ErrorCode::InvalidParamType, "Generated block id is empty"});
  }
  if (!IsValidGeneratedBlockId(data.id)) {
    return Result<GeneratedBlockData>(
        Error{ErrorCode::InvalidParamType,
              "Generated block id must use stable lowercase dot-separated segments"});
  }
  if (data.templ.empty()) {
    return Result<GeneratedBlockData>(Error{ErrorCode::InvalidParamType,
                                            "Generated block template is empty"});
  }

  const bool duplicate_in_request =
      !allow_id_collision &&
      std::ranges::find(existing_block_ids, data.id) != existing_block_ids.end();
  if (duplicate_in_request) {
    return Result<GeneratedBlockData>(
        Error{ErrorCode::DuplicateId, "Generated block id already exists"});
  }

  if (!allow_id_collision && block_repo != nullptr) {
    auto existing = block_repo->LoadLatest(data.id);
    if (!existing.HasError()) {
      return Result<GeneratedBlockData>(
          Error{ErrorCode::DuplicateId, "Generated block id already exists"});
    }
  }

  return Result<GeneratedBlockData>(std::move(data));
}

BlockDraft BuildGeneratedDraft(GeneratedBlockData data) {
  BlockDraftBuilder builder(data.id);
  builder.WithType(data.type)
      .WithLanguage(data.language.empty() ? "en" : data.language)
      .WithDescription(std::move(data.description))
      .WithTemplate(Template(std::move(data.templ)))
      .WithDefaults(std::move(data.defaults));

  for (const auto& tag : DeduplicateTags(data.tags)) {
    builder.WithTag(tag);
  }

  return builder.build();
}

std::string HashSuffix(const std::string& input) {
  std::ostringstream stream;
  stream << std::hex << std::hash<std::string>{}(input);
  auto value = stream.str();
  if (value.size() > 10) value.resize(10);
  return value;
}

std::string StyleKey(const SemanticStyle& style) {
  std::ostringstream stream;
  if (style.tone.has_value()) stream << "tone=" << *style.tone << ";";
  if (style.tense.has_value()) stream << "tense=" << *style.tense << ";";
  if (style.targetLanguage.has_value()) {
    stream << "lang=" << *style.targetLanguage << ";";
  }
  if (style.person.has_value()) stream << "person=" << *style.person << ";";
  if (style.rewriteStrength.has_value()) {
    stream << "rewrite=" << *style.rewriteStrength << ";";
  }
  if (style.audience.has_value()) stream << "audience=" << *style.audience << ";";
  if (style.locale.has_value()) stream << "locale=" << *style.locale << ";";
  if (style.terminologyRigidity.has_value()) {
    stream << "terms=" << *style.terminologyRigidity << ";";
  }
  if (style.preserveFormatting.has_value()) {
    stream << "formatting=" << (*style.preserveFormatting ? "true" : "false") << ";";
  }
  if (style.preserveExamples.has_value()) {
    stream << "examples=" << (*style.preserveExamples ? "true" : "false") << ";";
  }
  return stream.str();
}

std::string NormalizationKey(const SemanticStyle& style,
                             const std::string& fingerprint) {
  return HashSuffix(StyleKey(style) + "|" + fingerprint);
}

std::string NormalizationKeyTag(const std::string& key) {
  return "norm-key:" + key;
}

template <typename Container>
bool HasTag(const Container& tags, const std::string& tag) {
  return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

std::set<std::string> PlaceholderSet(const Template& templ) {
  const auto names = templ.ExtractParamNames();
  return std::set<std::string>(names.begin(), names.end());
}

std::string DerivedNormalizedBlockId(const Block& source_block,
                                     const SemanticStyle& style,
                                     const std::string& fingerprint) {
  (void)style;
  (void)fingerprint;
  return "norm." + source_block.Id();
}

std::string DerivedNormalizedCompositionId(const Composition& composition,
                                           const SemanticStyle& style,
                                           const std::string& fingerprint) {
  (void)style;
  (void)fingerprint;
  return "norm." + composition.id();
}

std::vector<std::string> SortedTags(
    const std::unordered_set<std::string>& tags) {
  std::vector<std::string> out(tags.begin(), tags.end());
  std::sort(out.begin(), out.end());
  return out;
}

}  // namespace

// Engine implementation

Engine::Engine() : config_{}, renderer_(std::make_unique<Renderer>()) {
  TF_LOG_INFO("Engine initialized with default config");
  FullInit();
}

Engine::Engine(EngineConfig config)
    : config_(std::move(config)), renderer_(std::make_unique<Renderer>()) {
  TF_LOG_INFO("Engine initialized with custom config [project={}, db_path={}]",
              config_.ProjectKey, config_.default_data_path);
  FullInit();
}

void Engine::SetBlockRepository(std::shared_ptr<IBlockRepository> repo) {
  blockRepo_ = std::move(repo);
  TF_LOG_DEBUG("Block repository set");

  // Create repository-backed block cache for renderer
  if (blockRepo_) {
    renderer_->SetBlockCache(
        std::make_unique<RepositoryBlockCache>(blockRepo_));
    TF_LOG_DEBUG("Renderer block cache initialized");
  }
}

void Engine::SetCompositionRepository(
    std::shared_ptr<ICompositionRepository> repo) {
  compRepo_ = std::move(repo);
  TF_LOG_DEBUG("Composition repository set");
}

void Engine::SetNormalizer(std::shared_ptr<INormalizer> normalizer) {
  normalizer_ = std::move(normalizer);
  TF_LOG_DEBUG("Normalizer set");
}

void Engine::SetBlockNormalizer(std::shared_ptr<IBlockNormalizer> normalizer) {
  blockNormalizer_ = std::move(normalizer);
  TF_LOG_DEBUG("Block normalizer set");
}

void Engine::SetBlockGenerator(std::shared_ptr<IBlockGenerator> generator) {
  blockGenerator_ = std::move(generator);
  TF_LOG_DEBUG("Block generator set");
}

void Engine::SetCompositionBlockRewriter(
    std::shared_ptr<ICompositionBlockRewriter> rewriter) {
  compositionBlockRewriter_ = std::move(rewriter);
  TF_LOG_DEBUG("Composition block rewriter set");
}

// ==================== Block Operations ====================

Result<Block> Engine::LoadBlock(const BlockId& id,
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
  return blockRepo_->LoadLatest(id);
}

Error Engine::DeprecateBlock(const BlockId& id, Version version) {
  TF_LOG_INFO("Deprecating block [id={}, version={}.{}]", id, version.major,
              version.minor);
  auto blockResult = LoadBlock(id, version);
  if (blockResult.HasError()) {
    TF_LOG_ERROR("Failed to deprecate block [id={}]: {}", id,
                 blockResult.error().message);
    return blockResult.error();
  }

  Block block = std::move(blockResult.value());
  block.deprecate();
  auto err = SaveBlock(block);
  if (err.is_error()) {
    TF_LOG_ERROR("Failed to save deprecated block [id={}]: {}", id,
                 err.message);
  } else {
    TF_LOG_INFO("Block deprecated successfully [id={}]", id);
  }
  return err;
}

Error Engine::DeleteBlock(const BlockId& id) {
  if (!blockRepo_) {
    TF_LOG_ERROR("Cannot delete block: no block repository configured");
    return Error{ErrorCode::StorageError, "No block repository configured"};
  }
  if (!compRepo_) {
    TF_LOG_ERROR("Cannot delete block: no composition repository configured");
    return Error{ErrorCode::StorageError, "No composition repository configured"};
  }

  for (const auto& composition_id : compRepo_->list()) {
    const auto versions_result = compRepo_->ListVersions(composition_id);
    if (versions_result.HasError()) {
      continue;
    }

    for (const auto& version : versions_result.value()) {
      auto composition_result = compRepo_->load(composition_id, version);
      if (composition_result.HasError()) {
        continue;
      }

      for (const auto& fragment : composition_result.value().fragments()) {
        if (fragment.IsBlockRef() && fragment.AsBlockRef().GetBlockId() == id) {
          return Error{
              ErrorCode::InvalidStateTransition,
              std::format("Block is used by composition {}@{}.{}",
                          composition_id, version.major, version.minor)};
        }
      }
    }
  }

  return blockRepo_->remove(id);
}

Result<Version> Engine::GetLatestBlockVersion(const BlockId& id) {
  if (!blockRepo_) {
    TF_LOG_ERROR(
        "Cannot get latest block version: no block repository configured");
    return Result<Version>(
        Error{ErrorCode::StorageError, "No block repository configured"});
  }
  TF_LOG_DEBUG("Getting latest block version [id={}]", id);
  return blockRepo_->GetLatestVersion(id);
}

Result<std::vector<Version>> Engine::ListBlockVersions(const BlockId& id) {
  if (!blockRepo_) {
    TF_LOG_ERROR("Cannot list block versions: no block repository configured");
    return Result<std::vector<Version>>(
        Error{ErrorCode::StorageError, "No block repository configured"});
  }
  TF_LOG_DEBUG("Listing block versions [id={}]", id);
  return blockRepo_->ListVersions(id);
}

std::vector<BlockId> Engine::ListBlocks(std::optional<BlockType> typeFilter) {
  if (!blockRepo_) {
    TF_LOG_WARN("Cannot list blocks: no block repository configured");
    return {};
  }
  TF_LOG_DEBUG("Listing blocks");
  return blockRepo_->list(typeFilter);
}

Result<Version> Engine::GetNextVersion(const BlockId& id, VersionBump bump) {
  auto current = blockRepo_->GetLatestVersion(id);

  Version next{1, 0};
  if (!current.HasError()) {
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

Result<PublishedBlock> Engine::PublishBlock(BlockDraft draft,
                                            VersionBump bump) {
  Block& block = draft.internal_;

  if (block.state() != BlockState::Draft) {
    return Result<PublishedBlock>(Error{ErrorCode::InvalidStateTransition,
                                        "Only Draft blocks can be published"});
  }

  auto next_ver_result = GetNextVersion(block.Id(), bump);
  if (next_ver_result.HasError()) {
    return Result<PublishedBlock>(next_ver_result.error());
  }

  return PublishBlockInternal(std::move(block), next_ver_result.value());
}

Result<PublishedBlock> Engine::PublishBlock(BlockDraft draft,
                                            Version explicit_version) {
  Block& block = draft.internal_;

  if (block.state() != BlockState::Draft) {
    return Result<PublishedBlock>(Error{ErrorCode::InvalidStateTransition,
                                        "Only Draft blocks can be published"});
  }

  auto existing = blockRepo_->load(block.Id(), explicit_version);
  if (!existing.HasError()) {
    return Result<PublishedBlock>(
        Error{ErrorCode::DuplicateId,
              std::format("Version {}.{} already exists",
                          explicit_version.major, explicit_version.minor)});
  }

  return PublishBlockInternal(std::move(block), explicit_version);
}

Result<PublishedBlock> Engine::UpdateBlock(BlockDraft draft, VersionBump bump) {
  if (!blockRepo_) {
    TF_LOG_ERROR("Cannot update block: no block repository configured");
    return Result<PublishedBlock>(
        Error{ErrorCode::StorageError, "No block repository configured"});
  }

  Block& block = draft.internal_;
  if (block.state() != BlockState::Draft) {
    return Result<PublishedBlock>(Error{ErrorCode::InvalidStateTransition,
                                        "Only Draft blocks can be published"});
  }

  auto existing = blockRepo_->LoadLatest(block.Id());
  if (existing.HasError()) {
    // Edit workflow is allowed only for already published block IDs.
    TF_LOG_WARN("Cannot update non-existing block [id={}]", block.Id());
    return Result<PublishedBlock>(Error::BlockNotFound(block.Id()));
  }

  return PublishBlock(std::move(draft), bump);
}

Result<GeneratedBlockData> Engine::GenerateBlockData(
    const BlockGenerationRequest& request) const {
  if (!blockGenerator_) {
    TF_LOG_ERROR("Cannot generate block data: no block generator configured");
    return Result<GeneratedBlockData>(
        Error{ErrorCode::StorageError, "No block generator configured"});
  }

  auto generated = blockGenerator_->GenerateBlock(request);
  if (generated.HasError()) {
    TF_LOG_WARN("Block generator failed: {}", generated.error().message);
    return Result<GeneratedBlockData>(generated.error());
  }

  return ValidateGeneratedBlockData(std::move(generated).value(),
                                    request.existing_block_ids,
                                    blockRepo_.get(),
                                    request.allow_id_collision);
}

Result<GeneratedBlockBatch> Engine::GenerateBlockBatchData(
    const PromptSlicingRequest& request) const {
  if (!blockGenerator_) {
    TF_LOG_ERROR("Cannot generate block batch: no block generator configured");
    return Result<GeneratedBlockBatch>(
        Error{ErrorCode::StorageError, "No block generator configured"});
  }

  auto generated = blockGenerator_->GenerateBlocks(request);
  if (generated.HasError()) {
    TF_LOG_WARN("Block generator batch failed: {}", generated.error().message);
    return Result<GeneratedBlockBatch>(generated.error());
  }

  auto batch = std::move(generated).value();
  if (batch.blocks.empty()) {
    return Result<GeneratedBlockBatch>(
        Error{ErrorCode::InvalidParamType, "Generated block batch is empty"});
  }

  std::vector<BlockId> existing_ids = request.existing_block_ids;
  std::vector<GeneratedBlockData> validated_blocks;
  validated_blocks.reserve(batch.blocks.size());
  for (auto& block : batch.blocks) {
    auto validated =
        ValidateGeneratedBlockData(std::move(block), existing_ids,
                                   blockRepo_.get(), request.allow_id_collision);
    if (validated.HasError()) {
      return Result<GeneratedBlockBatch>(validated.error());
    }
    existing_ids.push_back(validated.value().id);
    validated_blocks.push_back(std::move(validated).value());
  }

  return Result<GeneratedBlockBatch>(
      GeneratedBlockBatch{.blocks = std::move(validated_blocks)});
}

Result<BlockDraft> Engine::GenerateBlockDraft(
    const BlockGenerationRequest& request) const {
  auto generated = GenerateBlockData(request);
  if (generated.HasError()) {
    return Result<BlockDraft>(generated.error());
  }

  auto data = std::move(generated).value();
  TF_LOG_INFO("Generated block draft [id={}]", data.id);
  return Result<BlockDraft>(BuildGeneratedDraft(std::move(data)));
}

Result<std::vector<BlockDraft>> Engine::GenerateBlockDrafts(
    const PromptSlicingRequest& request) const {
  auto generated = GenerateBlockBatchData(request);
  if (generated.HasError()) {
    return Result<std::vector<BlockDraft>>(generated.error());
  }

  std::vector<BlockDraft> drafts;
  drafts.reserve(generated.value().blocks.size());
  for (auto& data : generated.value().blocks) {
    TF_LOG_INFO("Generated block draft [id={}]", data.id);
    drafts.push_back(BuildGeneratedDraft(std::move(data)));
  }

  return Result<std::vector<BlockDraft>>(std::move(drafts));
}

Result<PublishedBlock> Engine::PublishBlockInternal(Block block,
                                                    Version version) {
  auto err = block.publish(version);
  if (err.is_error()) {
    return Result<PublishedBlock>(err);
  }

  err = blockRepo_->save(block);
  if (err.is_error()) {
    return Result<PublishedBlock>(err);
  }

  TF_LOG_INFO("Block published: {} v{}", block.Id(), version);

  return Result<PublishedBlock>(PublishedBlock(block.Id(), version));
}

Result<Version> Engine::GetLatestCompositionVersion(const CompositionId& id) {
  if (!compRepo_) {
    TF_LOG_ERROR(
        "Cannot get latest composition version: no composition repository "
        "configured");
    return Result<Version>(
        Error{ErrorCode::StorageError, "No composition repository configured"});
  }

  TF_LOG_DEBUG("Getting latest composition version [id={}]", id);
  const auto version = compRepo_->GetLatestVersion(id);
  return version;
}

Result<std::vector<Version>> Engine::ListCompositionVersions(
    const CompositionId& id) {
  if (!compRepo_) {
    TF_LOG_ERROR(
        "Cannot list composition versions: no composition repository configured");
    return Result<std::vector<Version>>(
        Error{ErrorCode::StorageError, "No composition repository configured"});
  }

  TF_LOG_DEBUG("Listing composition versions [id={}]", id);
  return compRepo_->ListVersions(id);
}

Result<PublishedComposition> Engine::PublishComposition(CompositionDraft draft,
                                                        VersionBump bump) {
  Composition& comp = draft.internal_;

  if (comp.state() != BlockState::Draft) {
    return Result<PublishedComposition>(
        Error{ErrorCode::InvalidStateTransition,
              "Only Draft compositions can be published"});
  }

  for (const auto& frag : comp.fragments()) {
    if (frag.IsBlockRef()) {
      const auto& ref = frag.AsBlockRef();
      if (ref.UseLatest()) {
        return Result<PublishedComposition>(Error::VersionRequired());
      }
    }
  }

  Version next_version{1, 0};
  auto latest = compRepo_->GetLatestVersion(comp.id());
  if (!latest.HasError()) {
    if (bump == VersionBump::Major) {
      next_version =
          Version{static_cast<uint16_t>(latest.value().major + 1), 0};
    } else {
      next_version = Version{latest.value().major,
                             static_cast<uint16_t>(latest.value().minor + 1)};
    }
  }

  return PublishCompositionInternal(comp, next_version);
}

Result<PublishedComposition> Engine::PublishComposition(
    CompositionDraft draft, Version explicit_version) {
  Composition& comp = draft.internal_;

  if (comp.state() != BlockState::Draft) {
    return Result<PublishedComposition>(
        Error{ErrorCode::InvalidStateTransition,
              "Only Draft compositions can be published"});
  }

  if (auto existing = compRepo_->load(comp.id(), explicit_version);
      !existing.HasError()) {
    return Result<PublishedComposition>(
        Error{ErrorCode::DuplicateId, "Composition version already exists"});
  }

  for (const auto& frag : comp.fragments()) {
    if (frag.IsBlockRef() && frag.AsBlockRef().UseLatest()) {
      return Result<PublishedComposition>(Error::VersionRequired());
    }
  }

  return PublishCompositionInternal(std::move(comp), explicit_version);
}

Result<PublishedComposition> Engine::UpdateComposition(CompositionDraft draft,
                                                       VersionBump bump) {
  if (!compRepo_) {
    TF_LOG_ERROR("Cannot update composition: no composition repository configured");
    return Result<PublishedComposition>(
        Error{ErrorCode::StorageError, "No composition repository configured"});
  }

  Composition& comp = draft.internal_;
  if (comp.state() != BlockState::Draft) {
    return Result<PublishedComposition>(
        Error{ErrorCode::InvalidStateTransition,
              "Only Draft compositions can be published"});
  }

  auto existing = compRepo_->LoadLatest(comp.id());
  if (existing.HasError()) {
    TF_LOG_WARN("Cannot update non-existing composition [id={}]", comp.id());
    return Result<PublishedComposition>(Error::CompositionNotFound(comp.id()));
  }

  return PublishComposition(std::move(draft), bump);
}

Result<PublishedComposition> Engine::PublishCompositionInternal(
    Composition comp, Version version) {
  auto err = comp.publish(version);
  if (err.is_error()) return Result<PublishedComposition>(err);

  err = compRepo_->save(comp);
  if (err.is_error()) return Result<PublishedComposition>(err);

  return Result<PublishedComposition>(PublishedComposition(comp.id(), version));
}

Error Engine::SaveBlock(const Block& block) {
  if (!blockRepo_) {
    TF_LOG_ERROR("Cannot save block: no block repository configured");
    return Error{ErrorCode::StorageError, "No block repository configured"};
  }
  TF_LOG_INFO("Saving block [id={}, version={}.{}]", block.Id(),
              block.version().major, block.version().minor);
  return blockRepo_->save(block);
}

// ==================== Composition Operations ====================

Error Engine::SaveComposition(const Composition& composition) {
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

Result<Composition> Engine::LoadComposition(const CompositionId& id,
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
  return compRepo_->LoadLatest(id);
}

Error Engine::DeprecateComposition(const CompositionId& id, Version version) {
  TF_LOG_INFO("Deprecating composition [id={}, version={}.{}]", id,
              version.major, version.minor);
  auto compResult = LoadComposition(id, version);
  if (compResult.HasError()) {
    TF_LOG_ERROR("Failed to deprecate composition [id={}]: {}", id,
                 compResult.error().message);
    return compResult.error();
  }

  Composition comp = std::move(compResult.value());
  comp.deprecate();
  auto err = SaveComposition(comp);
  if (err.is_error()) {
    TF_LOG_ERROR("Failed to save deprecated composition [id={}]: {}", id,
                 err.message);
  } else {
    TF_LOG_INFO("Composition deprecated successfully [id={}]", id);
  }
  return err;
}

Error Engine::DeleteComposition(const CompositionId& id) {
  if (!compRepo_) {
    TF_LOG_ERROR("Cannot delete composition: no composition repository configured");
    return Error{ErrorCode::StorageError, "No composition repository configured"};
  }
  return compRepo_->remove(id);
}

std::vector<CompositionId> Engine::ListCompositions() const {
  if (!compRepo_) {
    TF_LOG_WARN(
        "Cannot list compositions: no composition repository configured");
    return {};
  }
  TF_LOG_DEBUG("Listing compositions");
  return compRepo_->list();
}

// ==================== Rendering Operations ====================

Result<RenderResult> Engine::Render(const CompositionId& compositionId,
                                    const RenderContext& context) {
  TF_LOG_INFO("Rendering composition [id={}]", compositionId);

  // Clear cache before rendering to ensure fresh data and prevent memory growth
  renderer_->ClearCache();

  auto compResult = LoadComposition(compositionId);
  if (compResult.HasError()) {
    TF_LOG_ERROR("Failed to render composition [id={}]: {}", compositionId,
                 compResult.error().message);
    return Result<RenderResult>(compResult.error());
  }
  return renderer_->Render(compResult.value(), context);
}

Result<RenderResult> Engine::Render(const CompositionId& compositionId,
                                    Version version,
                                    const RenderContext& context) {
  TF_LOG_INFO("Rendering composition [id={}, version={}.{}]", compositionId,
              version.major, version.minor);

  // Clear cache before rendering to ensure fresh data and prevent memory growth
  renderer_->ClearCache();

  auto compResult = LoadComposition(compositionId, version);
  if (compResult.HasError()) {
    TF_LOG_ERROR("Failed to render composition [id={}, version={}.{}]: {}",
                 compositionId, version.major, version.minor,
                 compResult.error().message);
    return Result<RenderResult>(compResult.error());
  }
  return renderer_->Render(compResult.value(), context);
}

Result<std::string> Engine::RenderBlock(const BlockId& blockId,
                                        const RenderContext& context) {
  TF_LOG_INFO("Rendering block [id={}]", blockId);

  // Clear cache before rendering to ensure fresh data and prevent memory growth
  renderer_->ClearCache();

  auto blockResult = LoadBlock(blockId);
  if (blockResult.HasError()) {
    TF_LOG_ERROR("Failed to render block [id={}]: {}", blockId,
                 blockResult.error().message);
    return Result<std::string>(blockResult.error());
  }
  return renderer_->RenderBlock(blockResult.value(), context);
}

Result<std::string> Engine::RenderBlock(const BlockId& blockId, Version version,
                                        const RenderContext& context) {
  TF_LOG_INFO("Rendering block [id={}, version={}.{}]", blockId, version.major,
              version.minor);

  // Clear cache before rendering to ensure fresh data and prevent memory growth
  renderer_->ClearCache();

  auto blockResult = LoadBlock(blockId, version);
  if (blockResult.HasError()) {
    TF_LOG_ERROR("Failed to render block [id={}, version={}.{}]: {}", blockId,
                 version.major, version.minor, blockResult.error().message);
    return Result<std::string>(blockResult.error());
  }
  return renderer_->RenderBlock(blockResult.value(), context);
}

// ==================== Normalization ====================

Result<std::string> Engine::Normalize(const std::string& text,
                                      const SemanticStyle& style) const {
  TF_LOG_DEBUG("Normalizing text (length={})", text.length());
  if (!normalizer_) {
    TF_LOG_ERROR("Cannot normalize: no normalizer configured");
    return Result<std::string>(
        Error{ErrorCode::StorageError, "No normalizer configured"});
  }
  return normalizer_->Normalize(text, style);
}

Result<NormalizedCompositionResult> Engine::NormalizeComposition(
    const CompositionNormalizationRequest& request) {
  TF_LOG_DEBUG("Normalizing composition [id={}]", request.source_composition_id);
  if (!blockNormalizer_) {
    TF_LOG_ERROR("Cannot normalize composition: no block normalizer configured");
    return Result<NormalizedCompositionResult>(
        Error{ErrorCode::StorageError, "No block normalizer configured"});
  }
  if (request.style.isEmpty()) {
    return Result<NormalizedCompositionResult>(
        Error{ErrorCode::InvalidParamType, "Semantic style is empty"});
  }

  auto composition_result =
      request.source_version.has_value()
          ? LoadComposition(request.source_composition_id, *request.source_version)
          : LoadComposition(request.source_composition_id);
  if (composition_result.HasError()) {
    return Result<NormalizedCompositionResult>(composition_result.error());
  }

  const Composition source = composition_result.value();
  const std::string normalization_key =
      NormalizationKey(request.style, blockNormalizer_->Fingerprint());
  const std::string derived_composition_id =
      request.target_composition_id.value_or(
          DerivedNormalizedCompositionId(source, request.style,
                                         blockNormalizer_->Fingerprint()));

  if (request.reuse_cached_blocks && compRepo_) {
    auto existing = compRepo_->LoadLatest(derived_composition_id);
    if (!existing.HasError() && existing.value().GetStyleProfile().has_value() &&
        StyleKey(existing.value().GetStyleProfile()->semantic) ==
            StyleKey(request.style)) {
      return Result<NormalizedCompositionResult>(NormalizedCompositionResult{
          .composition_id = derived_composition_id,
          .composition_version = existing.value().version(),
          .rewritten_blocks = {},
      });
    }
  }

  CompositionDraftBuilder builder(derived_composition_id);
  builder.WithProjectKey(source.ProjectKey())
      .WithDescription("Normalized derivative of " + source.id());
  auto style_profile =
      source.GetStyleProfile().value_or(StyleProfile::plain());
  style_profile.semantic = request.style;
  builder.WithStyleProfile(style_profile);

  std::vector<std::pair<BlockId, BlockId>> rewritten_blocks;

  for (const auto& fragment : source.fragments()) {
    if (fragment.IsSeparator()) {
      builder.AddSeparator(fragment.AsSeparator().type);
      continue;
    }

    if (fragment.IsStaticText()) {
      std::string text = fragment.AsStaticText().text();
      if (request.normalize_static_text && normalizer_) {
        auto normalized = normalizer_->Normalize(text, request.style);
        if (normalized.HasError()) {
          return Result<NormalizedCompositionResult>(normalized.error());
        }
        text = normalized.value();
      }
      builder.AddStaticText(std::move(text));
      continue;
    }

    const auto& block_ref = fragment.AsBlockRef();
    auto block_result = block_ref.version().has_value()
                            ? LoadBlock(block_ref.GetBlockId(), *block_ref.version())
                            : LoadBlock(block_ref.GetBlockId());
    if (block_result.HasError()) {
      return Result<NormalizedCompositionResult>(block_result.error());
    }

    const Block source_block = block_result.value();
    const std::string derived_block_id = DerivedNormalizedBlockId(
        source_block, request.style, blockNormalizer_->Fingerprint());
    const std::string normalization_key_tag =
        NormalizationKeyTag(normalization_key);

    PublishedBlock published_block(derived_block_id, Version{1, 0});
    bool reused_cached_block = false;
    if (request.reuse_cached_blocks && blockRepo_) {
      auto existing = blockRepo_->LoadLatest(derived_block_id);
      if (!existing.HasError() &&
          HasTag(existing.value().tags(), normalization_key_tag)) {
        published_block = PublishedBlock(existing.value().Id(),
                                         existing.value().version());
        reused_cached_block = true;
      }
    }

    if (!reused_cached_block) {
      auto normalized =
          blockNormalizer_->NormalizeBlock({.source_block = source_block,
                                           .style = request.style});
      if (normalized.HasError()) {
        return Result<NormalizedCompositionResult>(normalized.error());
      }

      const Template normalized_template(normalized.value().templ);
      if (PlaceholderSet(source_block.templ()) != PlaceholderSet(normalized_template)) {
        return Result<NormalizedCompositionResult>(
            Error{ErrorCode::InvalidParamType,
                  "Normalized block changed required placeholders"});
      }

      BlockDraftBuilder block_builder(derived_block_id);
      block_builder.WithType(source_block.type())
          .WithLanguage(normalized.value().language.value_or(source_block.language()))
          .WithDescription(
              normalized.value().description.value_or(source_block.description()))
          .WithTemplate(normalized_template)
          .WithDefaults(source_block.defaults())
          .WithParamSchema(source_block.param_schema());
      for (const auto& tag : source_block.tags()) {
        block_builder.WithTag(tag);
      }
      block_builder.WithTag("normalized");
      block_builder.WithTag(normalization_key_tag);

      Result<PublishedBlock> publish_result(Error{ErrorCode::StorageError, ""});
      if (blockRepo_->LoadLatest(derived_block_id).HasError()) {
        publish_result = PublishBlock(block_builder.build(), Version{1, 0});
      } else {
        publish_result = PublishBlock(block_builder.build(), VersionBump::Minor);
      }
      if (publish_result.HasError()) {
        return Result<NormalizedCompositionResult>(publish_result.error());
      }
      published_block = publish_result.value();
    }

    builder.AddBlockRef(published_block.ref().GetBlockId(),
                        published_block.version().major,
                        published_block.version().minor,
                        block_ref.LocalParams());
    rewritten_blocks.emplace_back(source_block.Id(), published_block.id());
  }

  Result<PublishedComposition> publish_composition(
      Error{ErrorCode::StorageError, ""});
  if (compRepo_->LoadLatest(derived_composition_id).HasError()) {
    publish_composition = PublishComposition(builder.build(), Version{1, 0});
  } else {
    publish_composition = PublishComposition(builder.build(), VersionBump::Minor);
  }
  if (publish_composition.HasError()) {
    return Result<NormalizedCompositionResult>(publish_composition.error());
  }

  return Result<NormalizedCompositionResult>(NormalizedCompositionResult{
      .composition_id = publish_composition.value().id(),
      .composition_version = publish_composition.value().version(),
      .rewritten_blocks = std::move(rewritten_blocks),
  });
}

Result<CompositionBlockRewritePreview> Engine::PreviewCompositionBlockRewrite(
    const CompositionBlockRewriteRequest& request) {
  TF_LOG_DEBUG("Previewing composition block rewrite [id={}]",
               request.source_composition_id);
  if (!compositionBlockRewriter_) {
    return Result<CompositionBlockRewritePreview>(
        Error{ErrorCode::StorageError,
              "No composition block rewriter configured"});
  }
  if (request.instruction.empty()) {
    return Result<CompositionBlockRewritePreview>(
        Error{ErrorCode::InvalidParamType, "Rewrite instruction is empty"});
  }

  auto composition_result =
      request.source_version.has_value()
          ? LoadComposition(request.source_composition_id, *request.source_version)
          : LoadComposition(request.source_composition_id);
  if (composition_result.HasError()) {
    return Result<CompositionBlockRewritePreview>(composition_result.error());
  }

  const Composition source = composition_result.value();
  CompositionBlockRewriteContext context{
      .source_composition_id = source.id(),
      .source_version = source.version(),
      .instruction = request.instruction,
      .preserve_language = request.preserve_language,
      .preserve_placeholders = request.preserve_placeholders,
  };

  std::unordered_set<BlockId> seen_block_ids;
  for (const auto& fragment : source.fragments()) {
    if (!fragment.IsBlockRef()) {
      continue;
    }
    const auto& block_ref = fragment.AsBlockRef();
    if (seen_block_ids.contains(block_ref.GetBlockId())) {
      continue;
    }

    auto block_result = block_ref.version().has_value()
                            ? LoadBlock(block_ref.GetBlockId(), *block_ref.version())
                            : LoadBlock(block_ref.GetBlockId());
    if (block_result.HasError()) {
      return Result<CompositionBlockRewritePreview>(block_result.error());
    }

    const Block& source_block = block_result.value();
    context.blocks.push_back(CompositionRewriteContextBlock{
        .block_id = source_block.Id(),
        .type = source_block.type(),
        .language = source_block.language(),
        .description = source_block.description(),
        .defaults = source_block.defaults(),
        .tags = SortedTags(source_block.tags()),
        .templ = source_block.templ().Content(),
    });
    seen_block_ids.insert(source_block.Id());
  }

  auto preview = compositionBlockRewriter_->PreviewRewrite(context);
  if (preview.HasError()) {
    return Result<CompositionBlockRewritePreview>(preview.error());
  }

  auto result = preview.value();
  result.source_composition_id = source.id();
  result.source_version = source.version();
  return Result<CompositionBlockRewritePreview>(std::move(result));
}

Result<AppliedCompositionBlockRewriteResult> Engine::ApplyCompositionBlockRewrite(
    const CompositionBlockRewritePreview& preview, const VersionBump bump) {
  TF_LOG_DEBUG("Applying composition block rewrite [id={}]",
               preview.source_composition_id);

  auto composition_result =
      LoadComposition(preview.source_composition_id, preview.source_version);
  if (composition_result.HasError()) {
    return Result<AppliedCompositionBlockRewriteResult>(
        composition_result.error());
  }
  const Composition source = composition_result.value();
  if (preview.patches.empty()) {
    return Result<AppliedCompositionBlockRewriteResult>(
        AppliedCompositionBlockRewriteResult{
            .composition_id = source.id(),
            .composition_version = source.version(),
            .rewritten_blocks = {},
        });
  }

  std::unordered_map<BlockId, BlockRewritePatch> patches_by_id;
  for (const auto& patch : preview.patches) {
    if (patch.block_id.empty()) {
      return Result<AppliedCompositionBlockRewriteResult>(
          Error{ErrorCode::InvalidParamType, "Rewrite patch block id is empty"});
    }
    if (patches_by_id.contains(patch.block_id)) {
      return Result<AppliedCompositionBlockRewriteResult>(
          Error{ErrorCode::InvalidParamType,
                "Duplicate rewrite patch for block " + patch.block_id});
    }
    patches_by_id.emplace(patch.block_id, patch);
  }

  std::unordered_map<BlockId, Block> source_blocks_by_id;
  for (const auto& fragment : source.fragments()) {
    if (!fragment.IsBlockRef()) {
      continue;
    }
    const auto& block_ref = fragment.AsBlockRef();
    if (source_blocks_by_id.contains(block_ref.GetBlockId())) {
      continue;
    }
    auto block_result = block_ref.version().has_value()
                            ? LoadBlock(block_ref.GetBlockId(), *block_ref.version())
                            : LoadBlock(block_ref.GetBlockId());
    if (block_result.HasError()) {
      return Result<AppliedCompositionBlockRewriteResult>(
          block_result.error());
    }
    source_blocks_by_id.emplace(block_ref.GetBlockId(), block_result.value());
  }

  std::unordered_map<BlockId, PublishedBlock> published_by_id;
  std::vector<std::pair<BlockId, Version>> rewritten_blocks;

  for (const auto& patch : preview.patches) {
    auto source_it = source_blocks_by_id.find(patch.block_id);
    if (source_it == source_blocks_by_id.end()) {
      return Result<AppliedCompositionBlockRewriteResult>(
          Error{ErrorCode::BlockNotFound,
                "Rewrite patch references block not present in source composition: " +
                    patch.block_id});
    }
    const Block& source_block = source_it->second;

    BlockDraftBuilder builder(source_block.Id());
    builder.WithType(source_block.type())
        .WithLanguage(source_block.language())
        .WithDescription(patch.description.value_or(source_block.description()))
        .WithTemplate(Template(patch.templ.value_or(
            source_block.templ().Content())))
        .WithDefaults(patch.defaults.value_or(source_block.defaults()))
        .WithParamSchema(source_block.param_schema())
        .WithRevisionComment(source_block.revision_comment());

    const auto tags = patch.tags.has_value()
                          ? std::unordered_set<std::string>(patch.tags->begin(),
                                                            patch.tags->end())
                          : source_block.tags();
    for (const auto& tag : tags) {
      builder.WithTag(tag);
    }

    const Template rewritten_template(
        patch.templ.value_or(source_block.templ().Content()));
    if (PlaceholderSet(source_block.templ()) !=
        PlaceholderSet(rewritten_template)) {
      return Result<AppliedCompositionBlockRewriteResult>(
          Error{ErrorCode::InvalidParamType,
                "Rewrite patch changed required placeholders for block " +
                    source_block.Id()});
    }

    auto publish_result = UpdateBlock(builder.build(), bump);
    if (publish_result.HasError()) {
      return Result<AppliedCompositionBlockRewriteResult>(
          publish_result.error());
    }
    published_by_id.emplace(source_block.Id(), publish_result.value());
    rewritten_blocks.emplace_back(source_block.Id(),
                                  publish_result.value().version());
  }

  CompositionDraftBuilder builder(source.id());
  builder.WithProjectKey(source.ProjectKey())
      .WithDescription(source.description())
      .WithRevisionComment(source.revision_comment());
  if (source.GetStyleProfile().has_value()) {
    builder.WithStyleProfile(*source.GetStyleProfile());
  }

  for (const auto& fragment : source.fragments()) {
    if (fragment.IsSeparator()) {
      builder.AddSeparator(fragment.AsSeparator().type);
      continue;
    }
    if (fragment.IsStaticText()) {
      builder.AddStaticText(fragment.AsStaticText().text());
      continue;
    }

    const auto& block_ref = fragment.AsBlockRef();
    auto rewritten = published_by_id.find(block_ref.GetBlockId());
    if (rewritten != published_by_id.end()) {
      builder.AddBlockRef(rewritten->second.ref().GetBlockId(),
                          rewritten->second.version().major,
                          rewritten->second.version().minor,
                          block_ref.LocalParams());
    } else {
      const auto version = block_ref.version().value_or(Version{0, 0});
      builder.AddBlockRef(block_ref.GetBlockId(), version.major, version.minor,
                          block_ref.LocalParams());
    }
  }

  auto publish_composition = UpdateComposition(builder.build(), bump);
  if (publish_composition.HasError()) {
    return Result<AppliedCompositionBlockRewriteResult>(
        publish_composition.error());
  }

  return Result<AppliedCompositionBlockRewriteResult>(
      AppliedCompositionBlockRewriteResult{
          .composition_id = publish_composition.value().id(),
          .composition_version = publish_composition.value().version(),
          .rewritten_blocks = std::move(rewritten_blocks),
      });
}

bool Engine::HasNormalizer() const noexcept { return normalizer_ != nullptr; }

bool Engine::HasBlockNormalizer() const noexcept {
  return blockNormalizer_ != nullptr;
}

bool Engine::HasBlockGenerator() const noexcept {
  return blockGenerator_ != nullptr;
}

bool Engine::HasCompositionBlockRewriter() const noexcept {
  return compositionBlockRewriter_ != nullptr;
}

// ==================== Validation ====================

Error Engine::ValidateComposition(const CompositionId& id) {
  TF_LOG_DEBUG("Validating composition [id={}]", id);
  auto compResult = LoadComposition(id);
  if (compResult.HasError()) {
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

Error Engine::ValidateBlock(const BlockId& id) {
  TF_LOG_DEBUG("Validating block [id={}]", id);
  auto blockResult = LoadBlock(id);
  if (blockResult.HasError()) {
    TF_LOG_ERROR("Failed to validate block [id={}]: {}", id,
                 blockResult.error().message);
    return blockResult.error();
  }
  auto block = blockResult.value();
  auto err = block.ValidateParams({}, {});
  if (err.is_error()) {
    TF_LOG_ERROR("Block validation failed [id={}]: {}", id, err.message);
  } else {
    TF_LOG_DEBUG("Block validated successfully [id={}]", id);
  }
  return err;
}

void Engine::FullInit() {
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

  SetBlockRepository(block_repository);
  SetCompositionRepository(composition_repository);
}
}  // namespace tf
