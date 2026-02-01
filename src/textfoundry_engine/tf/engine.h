//
// Created by artem.d on 28.01.2026.
//

#pragma once

#include "block.h"
#include "composition.h"
#include "renderer.h"
#include "error.h"

#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace tf {
  // Forward declarations for repository interfaces
  class IBlockRepository;
  class ICompositionRepository;

  /**
   * Engine configuration
   */
  struct EngineConfig {
    std::string project_key = "default";
    bool strict_mode = false; ///< Default strict mode for rendering
    std::string default_data_path = "memory:tf"; ///< Default data path for block and composition repositories
  };

  /**
   * Normalizer interface for semantic text transformation (LLM)
   * NOT applied automatically - requires explicit normalize() call
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
    [[nodiscard]] virtual Result<std::string> normalize(
      const std::string &text,
      const SemanticStyle &style
    ) const = 0;
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
    void set_block_repository(std::shared_ptr<IBlockRepository> repo);

    /**
     * Set composition repository for persistence
     */
    void set_composition_repository(std::shared_ptr<ICompositionRepository> repo);

    /**
     * Set normalizer for semantic transformations
     */
    void set_normalizer(std::shared_ptr<INormalizer> normalizer);

    // ==================== Block Operations ====================

    /**
     * Load block from storage
     */
    [[nodiscard]] Result<Block> load_block(const BlockId &id, std::optional<Version> version = std::nullopt) const;


    /**
     * Deprecate a published block
     */
    [[nodiscard]] Error deprecate_block(const BlockId &id, Version version);

    /**
     * Get latest version of a block
     */
    [[nodiscard]] Result<Version> get_latest_block_version(const BlockId &id);

    /**
     * List all blocks (optionally filtered by type)
     */
    [[nodiscard]] std::vector<BlockId> list_blocks(std::optional<BlockType> typeFilter = std::nullopt);


    [[nodiscard]] Result<PublishedBlock> publish_block(
      BlockDraft draft,
      VersionBump bump = VersionBump::Minor
    );

    [[nodiscard]] Result<PublishedBlock> publish_block(
      BlockDraft draft,
      Version explicit_version
    );


    // ==================== Composition Operations ====================


    [[nodiscard]] Result<PublishedComposition> publish_composition(
      CompositionDraft draft,
      VersionBump bump = VersionBump::Minor
    );

    [[nodiscard]] Result<PublishedComposition> publish_composition(
      CompositionDraft draft,
      Version explicit_version
    );

    Result<Version> get_latest_composition_version(const CompositionId &id);

    /**
     * Load composition from storage
     */
    [[nodiscard]] Result<Composition> load_composition(
      const CompositionId &id,
      std::optional<Version> version = std::nullopt
    );

    /**
     * Deprecate a published composition
     */
    [[nodiscard]] Error deprecate_composition(const CompositionId &id, Version version);

    /**
     * List all compositions
     */
    [[nodiscard]] std::vector<CompositionId> list_compositions() const;

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
    [[nodiscard]] Result<RenderResult> render(
      const CompositionId &compositionId,
      const RenderContext &context = RenderContext{}
    );


    /**
     * Render a specific version of a composition
     */
    [[nodiscard]] Result<RenderResult> render(
      const CompositionId &compositionId,
      Version version,
      const RenderContext &context = RenderContext{}
    );

    /**
     * Render a single block (for preview/testing)
     */
    [[nodiscard]] Result<std::string> render_block(
      const BlockId &blockId,
      const RenderContext &context = RenderContext{}
    );

    /**
     * Render block with specific version
     */
    [[nodiscard]] Result<std::string> render_block(
      const BlockId &blockId,
      Version version,
      const RenderContext &context = RenderContext{}
    );

    // ==================== Normalization (Optional LLM) ====================

    /**
     * Normalize rendered text using configured Normalizer
     * Requires explicit call - NOT applied automatically during render()
     *
     * @param text Text to normalize
     * @param style Semantic style parameters
     * @returns Normalized text or Error (if no Normalizer configured)
     */
    [[nodiscard]] Result<std::string> normalize(
      const std::string &text,
      const SemanticStyle &style
    ) const;

    /**
     * Check if normalizer is configured
     */
    [[nodiscard]] bool hasNormalizer() const noexcept;

    // ==================== Full Initialization ====================

    /**
     * Initialize engine with ObjectBox repositories
     * Uses config_.defaultDataPath as database directory
     */
    void full_init();

    // ==================== Validation ====================

    /**
     * Validate a composition without rendering
     */
    [[nodiscard]] Error validate_composition(const CompositionId &id);

    /**
     * Validate a block
     */
    [[nodiscard]] Error validate_block(const BlockId &id);

  private:
    EngineConfig config_;
    std::shared_ptr<IBlockRepository> blockRepo_;
    std::shared_ptr<ICompositionRepository> compRepo_;
    std::shared_ptr<INormalizer> normalizer_;
    std::unique_ptr<Renderer> renderer_;

    Result<PublishedComposition> publish_composition_internal(Composition comp, Version version);

    Result<PublishedBlock> publish_block_internal(Block block, Version version);

    /**
     * Save block draft to storage
     */
    [[nodiscard]] Error save_block(const Block &block);

    /**
     * Save composition draft to storage
     */
    [[nodiscard]] Error save_composition(const Composition &composition);


    // ==================== Helpers ====================
    Result<Version> get_next_version(const BlockId &id, VersionBump bump);
  };

  // ==================== Repository Interfaces ====================

  /**
   * Block repository interface (port)
   * Implementation depends on storage backend (ObjectBox, etc.)
   */
  class IBlockRepository {
  public:
    virtual ~IBlockRepository() = default;

    [[nodiscard]] virtual Error save(const Block &block) = 0;

    [[nodiscard]] virtual Result<Block> load(const BlockId &id, Version version) = 0;

    [[nodiscard]] virtual Result<Block> load_latest(const BlockId &id) = 0;

    [[nodiscard]] virtual std::vector<BlockId> list(std::optional<BlockType> typeFilter = std::nullopt) = 0;

    [[nodiscard]] virtual Result<Version> get_latest_version(const BlockId &id) = 0;

    [[nodiscard]] virtual Error deprecate(const BlockId &id, Version version) = 0;
  };

  /**
   * Composition repository interface (port)
   * Implementation depends on storage backend
   */
  class ICompositionRepository {
  public:
    virtual ~ICompositionRepository() = default;

    [[nodiscard]] virtual Error save(const Composition &composition) = 0;

    [[nodiscard]] virtual Result<Composition> load(const CompositionId &id, Version version) = 0;

    [[nodiscard]] virtual Result<Composition> load_latest(const CompositionId &id) = 0;

    [[nodiscard]] virtual std::vector<CompositionId> list() = 0;

    [[nodiscard]] virtual Result<Version> get_latest_version(const CompositionId &id) = 0;

    [[nodiscard]] virtual Error deprecate(const CompositionId &id, Version version) = 0;
  };
} // namespace tf
