//
// Created by artem.d on 28.01.2026.
//

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "block.h"
#include "block_generation.h"
#include "composition.h"
#include "error.h"
#include "renderer.h"

namespace tf {
// Forward declarations for repository interfaces
class IBlockRepository;
class ICompositionRepository;

/**
 * Engine configuration
 */
struct EngineConfig {
  std::string ProjectKey = "default";
  bool strict_mode = false;  ///< Default strict mode for rendering
  std::string default_data_path =
      "memory:tf";  ///< Default data path for block and composition
                    ///< repositories
};

/**
 * Normalizer interface for semantic text transformation (LLM)
 * NOT applied automatically - requires explicit Normalize() call
 */
class INormalizer {
 public:
  virtual ~INormalizer() = default;

  /**
   * Normalize text according to semantic style
   * @param text Input text
   * @param style Semantic style parameters
   * @returns Normalized text or Error
   */
  [[nodiscard]] virtual Result<std::string> Normalize(
      const std::string& text, const SemanticStyle& style) const = 0;
};

struct BlockNormalizationRequest {
  Block source_block;
  SemanticStyle style;
};

struct NormalizedBlockData {
  std::string templ;
  std::optional<std::string> description;
  std::optional<std::string> language;
};

class IBlockNormalizer {
 public:
  virtual ~IBlockNormalizer() = default;

  [[nodiscard]] virtual Result<NormalizedBlockData> NormalizeBlock(
      const BlockNormalizationRequest& request) const = 0;

  [[nodiscard]] virtual std::string Fingerprint() const = 0;
};

struct CompositionNormalizationRequest {
  CompositionId source_composition_id;
  std::optional<Version> source_version;
  SemanticStyle style;
  std::optional<std::string> target_composition_id;
  bool normalize_static_text = false;
  bool reuse_cached_blocks = true;
};

struct NormalizedCompositionResult {
  CompositionId composition_id;
  Version composition_version;
  std::vector<std::pair<BlockId, BlockId>> rewritten_blocks;
};

/**
 * TextEngine - main API class for TextFoundry
 *
 * Provides:
 * - Block and Composition lifecycle management
 * - Rendering with Template Expansion
 * - Optional normalization via external Normalizer
 *
 * Stateless except for repository connections
 */
class Engine {
 public:
  enum class VersionBump { Minor, Major };

  Engine();

  explicit Engine(EngineConfig config);

  /**
   * Set block repository for persistence
   */
  void SetBlockRepository(std::shared_ptr<IBlockRepository> repo);

  /**
   * Set composition repository for persistence
   */
  void SetCompositionRepository(std::shared_ptr<ICompositionRepository> repo);

  /**
   * Set normalizer for semantic transformations
   */
  void SetNormalizer(std::shared_ptr<INormalizer> normalizer);

  /**
   * Set block normalizer for structure-preserving block rewrites.
   */
  void SetBlockNormalizer(std::shared_ptr<IBlockNormalizer> normalizer);

  /**
   * Set AI-assisted block generator for explicit authoring workflows.
   */
  void SetBlockGenerator(std::shared_ptr<IBlockGenerator> generator);

  // ==================== Block Operations ====================

  /**
   * Load block from storage
   */
  [[nodiscard]] Result<Block> LoadBlock(
      const BlockId& id, std::optional<Version> version = std::nullopt) const;

  /**
   * Deprecate a published block
   */
  [[nodiscard]] Error DeprecateBlock(const BlockId& id, Version version);

  /**
   * Get latest version of a block
   */
  [[nodiscard]] Result<Version> GetLatestBlockVersion(const BlockId& id);

  /**
   * List all versions of a block, newest first.
   */
  [[nodiscard]] Result<std::vector<Version>> ListBlockVersions(
      const BlockId& id);

  /**
   * List all blocks (optionally filtered by type)
   */
  [[nodiscard]] std::vector<BlockId> ListBlocks(
      std::optional<BlockType> typeFilter = std::nullopt);

  [[nodiscard]] Result<PublishedBlock> PublishBlock(
      BlockDraft draft, VersionBump bump = VersionBump::Minor);

  [[nodiscard]] Result<PublishedBlock> PublishBlock(BlockDraft draft,
                                                    Version explicit_version);

  /**
   * Publish a new version of an existing block (edit workflow).
   * Returns BlockNotFound when the target block does not exist yet.
   */
  [[nodiscard]] Result<PublishedBlock> UpdateBlock(
      BlockDraft draft, VersionBump bump = VersionBump::Minor);

  /**
   * Generate structured block data through the configured block generator.
   */
  [[nodiscard]] Result<GeneratedBlockData> GenerateBlockData(
      const BlockGenerationRequest& request) const;

  /**
   * Generate multiple structured block candidates by decomposing a source
   * prompt into reusable blocks.
   */
  [[nodiscard]] Result<GeneratedBlockBatch> GenerateBlockBatchData(
      const PromptSlicingRequest& request) const;

  /**
   * Generate a block draft through the configured block generator.
   *
   * This never publishes automatically. Callers are expected to review and
   * publish the returned draft explicitly.
   */
  [[nodiscard]] Result<BlockDraft> GenerateBlockDraft(
      const BlockGenerationRequest& request) const;

  /**
   * Generate block drafts by decomposing a source prompt into reusable blocks.
   */
  [[nodiscard]] Result<std::vector<BlockDraft>> GenerateBlockDrafts(
      const PromptSlicingRequest& request) const;

  // ==================== Composition Operations ====================

  [[nodiscard]] Result<PublishedComposition> PublishComposition(
      CompositionDraft draft, VersionBump bump = VersionBump::Minor);

  [[nodiscard]] Result<PublishedComposition> PublishComposition(
      CompositionDraft draft, Version explicit_version);

  /**
   * Publish a new version of an existing composition (edit workflow).
   * Returns CompositionNotFound when the target composition does not exist yet.
   */
  [[nodiscard]] Result<PublishedComposition> UpdateComposition(
      CompositionDraft draft, VersionBump bump = VersionBump::Minor);

  Result<Version> GetLatestCompositionVersion(const CompositionId& id);

  [[nodiscard]] Result<std::vector<Version>> ListCompositionVersions(
      const CompositionId& id);

  /**
   * Load composition from storage
   */
  [[nodiscard]] Result<Composition> LoadComposition(
      const CompositionId& id, std::optional<Version> version = std::nullopt);

  /**
   * Deprecate a published composition
   */
  [[nodiscard]] Error DeprecateComposition(const CompositionId& id,
                                           Version version);

  /**
   * List all compositions
   */
  [[nodiscard]] std::vector<CompositionId> ListCompositions() const;

  // ==================== Rendering Operations ====================

  /**
   * Render composition to text
   *
   * Processing:
   * 1. Template Expansion (parameter substitution)
   * 2. StructuralStyle Application (formatting)
   *
   * @param compositionId Composition to render
   * @param context Runtime parameters
   * @returns RenderResult or Error
   */
  [[nodiscard]] Result<RenderResult> Render(
      const CompositionId& compositionId,
      const RenderContext& context = RenderContext{});

  /**
   * Render a specific version of a composition
   */
  [[nodiscard]] Result<RenderResult> Render(
      const CompositionId& compositionId, Version version,
      const RenderContext& context = RenderContext{});

  /**
   * Render a single block (for preview/testing)
   */
  [[nodiscard]] Result<std::string> RenderBlock(
      const BlockId& blockId, const RenderContext& context = RenderContext{});

  /**
   * Render block with specific version
   */
  [[nodiscard]] Result<std::string> RenderBlock(
      const BlockId& blockId, Version version,
      const RenderContext& context = RenderContext{});

  // ==================== Normalization (Optional LLM) ====================

  /**
   * Normalize rendered text using configured Normalizer
   * Requires explicit call - NOT applied automatically during Render()
   *
   * @param text Text to normalize
   * @param style Semantic style parameters
   * @returns Normalized text or Error (if no Normalizer configured)
   */
  [[nodiscard]] Result<std::string> Normalize(const std::string& text,
                                              const SemanticStyle& style) const;

  /**
   * Create a derived composition by normalizing individual block templates.
   *
   * The original composition structure is preserved. Block refs are rewritten
   * to newly published normalized block versions or cached derived blocks.
   */
  [[nodiscard]] Result<NormalizedCompositionResult> NormalizeComposition(
      const CompositionNormalizationRequest& request);

  /**
   * Check if normalizer is configured
   */
  [[nodiscard]] bool HasNormalizer() const noexcept;

  /**
   * Check if a block normalizer is configured.
   */
  [[nodiscard]] bool HasBlockNormalizer() const noexcept;

  /**
   * Check if a block generator is configured.
   */
  [[nodiscard]] bool HasBlockGenerator() const noexcept;

  // ==================== Full Initialization ====================

  /**
   * Initialize engine with ObjectBox repositories
   * Uses config_.defaultDataPath as database directory
   */
  void FullInit();

  // ==================== Validation ====================

  /**
   * Validate a composition without rendering
   */
  [[nodiscard]] Error ValidateComposition(const CompositionId& id);

  /**
   * Validate a block
   */
  [[nodiscard]] Error ValidateBlock(const BlockId& id);

 private:
  EngineConfig config_;
  std::shared_ptr<IBlockRepository> blockRepo_;
  std::shared_ptr<ICompositionRepository> compRepo_;
  std::shared_ptr<INormalizer> normalizer_;
  std::shared_ptr<IBlockNormalizer> blockNormalizer_;
  std::shared_ptr<IBlockGenerator> blockGenerator_;
  std::unique_ptr<Renderer> renderer_;

  Result<PublishedComposition> PublishCompositionInternal(Composition comp,
                                                          Version version);

  Result<PublishedBlock> PublishBlockInternal(Block block, Version version);

  /**
   * Save block draft to storage
   */
  [[nodiscard]] Error SaveBlock(const Block& block);

  /**
   * Save composition draft to storage
   */
  [[nodiscard]] Error SaveComposition(const Composition& composition);

  // ==================== Helpers ====================
  Result<Version> GetNextVersion(const BlockId& id, VersionBump bump);
};

// ==================== Repository Interfaces ====================

/**
 * Block repository interface (port)
 * Implementation depends on storage backend (ObjectBox, etc.)
 */
class IBlockRepository {
 public:
  virtual ~IBlockRepository() = default;

  [[nodiscard]] virtual Error save(const Block& block) = 0;

  [[nodiscard]] virtual Result<Block> load(const BlockId& id,
                                           Version version) = 0;

  [[nodiscard]] virtual Result<Block> LoadLatest(const BlockId& id) = 0;

  [[nodiscard]] virtual std::vector<BlockId> list(
      std::optional<BlockType> typeFilter = std::nullopt) = 0;

  [[nodiscard]] virtual Result<Version> GetLatestVersion(const BlockId& id) = 0;

  [[nodiscard]] virtual Result<std::vector<Version>> ListVersions(
      const BlockId& id) = 0;

  [[nodiscard]] virtual Error deprecate(const BlockId& id, Version version) = 0;
};

/**
 * Composition repository interface (port)
 * Implementation depends on storage backend
 */
class ICompositionRepository {
 public:
  virtual ~ICompositionRepository() = default;

  [[nodiscard]] virtual Error save(const Composition& composition) = 0;

  [[nodiscard]] virtual Result<Composition> load(const CompositionId& id,
                                                 Version version) = 0;

  [[nodiscard]] virtual Result<Composition> LoadLatest(
      const CompositionId& id) = 0;

  [[nodiscard]] virtual std::vector<CompositionId> list() = 0;

  [[nodiscard]] virtual Result<Version> GetLatestVersion(
      const CompositionId& id) = 0;

  [[nodiscard]] virtual Result<std::vector<Version>> ListVersions(
      const CompositionId& id) = 0;

  [[nodiscard]] virtual Error deprecate(const CompositionId& id,
                                        Version version) = 0;
};
}  // namespace tf
