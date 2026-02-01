//
// Created by a.durynin on 29.01.2026.
//

#pragma once

#include "fragment.h"
#include "block.h"
#include "version.h"
#include "blocktype.hpp"
#include "error.h"

#include <string>
#include <vector>
#include <optional>

#include <rfl/json.hpp>

namespace tf {
  /**
   * CompositionId - unique identifier for a composition
   */
  using CompositionId = std::string;

  /**
   * RenderContext - runtime parameters for rendering
   * Passed to render() method, highest priority in parameter resolution
   */
  struct RenderContext {
    Params params; ///< Runtime parameter overrides
    std::optional<std::string> targetLanguage; ///< Target language for output
    bool strictMode = false; ///< If true, fail on missing params; if false, use defaults

    /**
     * Add a runtime parameter
     */
    RenderContext &with_param(const std::string &name, ParamValue value);

    /**
     * Set target language
     */
    RenderContext &with_language(std::string lang);

    /**
     * Set strict mode
     */
    RenderContext &with_strict_mode(bool strict);
  };

  /**
   * StructuralStyle - deterministic formatting parameters
   * Applied automatically during rendering, doesn't modify semantics
   */
  struct StructuralStyle {
    std::optional<std::string> blockWrapper; ///< Template with {{content}} placeholder
    std::optional<std::string> preamble; ///< Text before composition
    std::optional<std::string> postamble; ///< Text after composition
    std::optional<std::string> delimiter; ///< Separator between fragments
  };

  /**
   * SemanticStyle - parameters requiring text rewriting (LLM)
   * NOT applied automatically - requires explicit normalize() call
   */
  struct SemanticStyle {
    std::optional<std::string> tone; ///< e.g., "formal", "casual"
    std::optional<std::string> tense; ///< e.g., "past", "present"
    std::optional<std::string> targetLanguage; ///< e.g., "en", "ru"
    std::optional<std::string> person; ///< e.g., "first", "third"

    [[nodiscard]] bool isEmpty() const noexcept;
  };

  /**
   * StyleProfile - combined structural and semantic styles
   */
  struct StyleProfile {
    StructuralStyle structural;
    SemanticStyle semantic;

    [[nodiscard]] static StyleProfile plain();
  };

  /**
   * Composition - ordered list of fragments that forms a complete text
   * Consists of BlockRef, StaticText, and Separator fragments
   */
  class Composition {
  public:
    Composition() = default;

    explicit Composition(CompositionId id);

    // Getters
    [[nodiscard]] const CompositionId &id() const noexcept;

    [[nodiscard]] BlockState state() const noexcept;

    [[nodiscard]] const Version &version() const noexcept;

    void set_version(const Version &v);

    [[nodiscard]] const std::vector<Fragment> &fragments() const noexcept;

    [[nodiscard]] const std::optional<StyleProfile> &style_profile() const noexcept;

    [[nodiscard]] const std::string &project_key() const noexcept;

    [[nodiscard]] const std::string &description() const noexcept;

    // Setters (allowed only for Draft state)
    void set_id(CompositionId id);

    void set_state(BlockState state);

    void set_style_profile(StyleProfile profile);

    void set_project_key(std::string key);

    void set_description(std::string desc);

    /**
     * Add a BlockRef fragment with local parameters
     * @param blockId Block identifier
     * @param version Block version (mandatory for reproducibility)
     * @param localParams Local parameter overrides
     */
    Fragment &add_block_ref(const BlockId &blockId, Version version, Params localParams = {});

    /**
     * Add a BlockRef with use_latest flag (Draft only)
     */
    Fragment &add_block_ref_latest(const BlockId &blockId, Params localParams = {});

    /**
     * Add static text fragment
     */
    Fragment &add_static_text(std::string text);

    /**
     * Add typed separator
     */
    Fragment &add_separator(SeparatorType type);

    /**
     * Insert fragment at specific index
     */
    void insert_fragment(size_t index, Fragment fragment);

    /**
     * Remove fragment at index
     */
    void remove_fragment(size_t index);

    /**
     * Clear all fragments
     */
    void clear_fragments();

    /**
     * Access fragment by index
     */
    [[nodiscard]] Fragment &fragment(size_t index);

    [[nodiscard]] const Fragment &fragment(size_t index) const;

    [[nodiscard]] size_t fragmentCount() const noexcept;

    /**
     * Validate composition
     * @returns Error if invalid (e.g., BlockRef without version in non-draft)
     */
    [[nodiscard]] Error validate() const;

    /**
     * Publish this composition - creates immutable version
     * @param new_version version to assign
     * @returns Error if cannot be published
     */
    [[nodiscard]] Error publish(Version new_version);

    /**
     * Mark this version as deprecated
     */
    void deprecate();

  private:
    // Identity
    CompositionId id_;
    BlockState state_ = BlockState::Draft;
    Version version_{0, 0};
    std::string project_key_ = "default";

    // Content
    std::vector<Fragment> fragments_;
    std::optional<StyleProfile> style_profile_;

    // Metadata
    std::string description_;
  };

  class CompositionDraft {
    friend class Engine;
    friend class CompositionDraftBuilder;

  public:
    CompositionDraft(CompositionDraft &&) = default;

  private:
    Composition internal_;

    explicit CompositionDraft(Composition &&c);
  };

  class PublishedComposition {
    friend class Engine;

  public:
    CompositionId id() const noexcept;

    Version version() const noexcept;

  private:
    CompositionId id_;
    Version version_;

    explicit PublishedComposition(CompositionId id, Version ver);
  };

  /**
 * Builder for creating Composition drafts
 */
  class CompositionDraftBuilder {
  public:
    CompositionDraftBuilder() = default;

    explicit CompositionDraftBuilder(CompositionId id);

    CompositionDraftBuilder &with_id(CompositionId id);

    CompositionDraftBuilder &with_style_profile(StyleProfile profile);

    CompositionDraftBuilder &with_project_key(std::string key);

    CompositionDraftBuilder &with_description(std::string desc);

    CompositionDraftBuilder &add_block_ref(BlockRef ref);

    CompositionDraftBuilder &add_static_text(std::string text);

    CompositionDraftBuilder &add_separator(SeparatorType type);

    [[nodiscard]] CompositionDraft build();


  private:
    Composition comp_;
  };
} // namespace tf
