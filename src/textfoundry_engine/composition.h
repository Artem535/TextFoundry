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
    Params params;  ///< Runtime parameter overrides
    std::optional<std::string> targetLanguage;  ///< Target language for output
    bool strictMode = false;  ///< If true, fail on missing params; if false, use defaults

    /**
     * Add a runtime parameter
     */
    RenderContext& withParam(const std::string& name, ParamValue value);

    /**
     * Set target language
     */
    RenderContext& withLanguage(std::string lang);

    /**
     * Set strict mode
     */
    RenderContext& withStrictMode(bool strict);
};

/**
 * StructuralStyle - deterministic formatting parameters
 * Applied automatically during rendering, doesn't modify semantics
 */
struct StructuralStyle {
    enum class OutputFormat {
        Plain,
        Markdown,
        Json,
        Xml
    };

    OutputFormat outputFormat = OutputFormat::Plain;
    std::optional<std::string> blockWrapper;  ///< Template with {{content}} placeholder
    std::optional<std::string> preamble;      ///< Text before composition
    std::optional<std::string> postamble;     ///< Text after composition
    std::optional<std::string> delimiter;     ///< Separator between fragments

    [[nodiscard]] static StructuralStyle plain();
    [[nodiscard]] static StructuralStyle markdown();
    [[nodiscard]] static StructuralStyle json();
};

/**
 * SemanticStyle - parameters requiring text rewriting (LLM)
 * NOT applied automatically - requires explicit normalize() call
 */
struct SemanticStyle {
    std::optional<std::string> tone;           ///< e.g., "formal", "casual"
    std::optional<std::string> tense;          ///< e.g., "past", "present"
    std::optional<std::string> targetLanguage; ///< e.g., "en", "ru"
    std::optional<std::string> person;         ///< e.g., "first", "third"

    [[nodiscard]] bool isEmpty() const noexcept;
};

/**
 * StyleProfile - combined structural and semantic styles
 */
struct StyleProfile {
    StructuralStyle structural;
    SemanticStyle semantic;

    [[nodiscard]] static StyleProfile plain();
    [[nodiscard]] static StyleProfile markdown();
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
    [[nodiscard]] const CompositionId& id() const noexcept;
    [[nodiscard]] BlockState state() const noexcept;
    [[nodiscard]] const Version& version() const noexcept;
    [[nodiscard]] const std::vector<Fragment>& fragments() const noexcept;
    [[nodiscard]] const std::optional<StyleProfile>& styleProfile() const noexcept;
    [[nodiscard]] const std::string& projectKey() const noexcept;
    [[nodiscard]] const std::string& description() const noexcept;

    // Setters (allowed only for Draft state)
    void setId(CompositionId id);
    void setStyleProfile(StyleProfile profile);
    void setProjectKey(std::string key);
    void setDescription(std::string desc);

    /**
     * Add a BlockRef fragment with local parameters
     * @param blockId Block identifier
     * @param version Block version (mandatory for reproducibility)
     * @param localParams Local parameter overrides
     */
    Fragment& addBlockRef(const BlockId& blockId, Version version, Params localParams = {});

    /**
     * Add a BlockRef with use_latest flag (Draft only)
     */
    Fragment& addBlockRefLatest(const BlockId& blockId, Params localParams = {});

    /**
     * Add static text fragment
     */
    Fragment& addStaticText(std::string text);

    /**
     * Add typed separator
     */
    Fragment& addSeparator(SeparatorType type);

    /**
     * Insert fragment at specific index
     */
    void insertFragment(size_t index, Fragment fragment);

    /**
     * Remove fragment at index
     */
    void removeFragment(size_t index);

    /**
     * Clear all fragments
     */
    void clearFragments();

    /**
     * Access fragment by index
     */
    [[nodiscard]] Fragment& fragment(size_t index);
    [[nodiscard]] const Fragment& fragment(size_t index) const;

    [[nodiscard]] size_t fragmentCount() const noexcept;

    /**
     * Validate composition
     * @returns Error if invalid (e.g., BlockRef without version in non-draft)
     */
    [[nodiscard]] Error validate() const;

    /**
     * Publish this composition - creates immutable version
     * @param newVersion version to assign
     * @returns Error if cannot be published
     */
    [[nodiscard]] Error publish(Version newVersion);

    /**
     * Mark this version as deprecated
     */
    void deprecate();

    /**
     * Serialize fragments to JSON for storage
     */
    [[nodiscard]] std::string serializeFragments() const;

    /**
     * Deserialize fragments from JSON
     */
    [[nodiscard]] Error deserializeFragments(const std::string& json);

private:
    // Identity
    CompositionId id_;
    BlockState state_ = BlockState::Draft;
    Version version_{0, 0};
    std::string projectKey_ = "default";

    // Content
    std::vector<Fragment> fragments_;
    std::optional<StyleProfile> styleProfile_;

    // Metadata
    std::string description_;
};

/**
 * Builder for creating Composition drafts
 */
class CompositionDraftBuilder {
public:
    CompositionDraftBuilder();
    explicit CompositionDraftBuilder(CompositionId id);

    CompositionDraftBuilder& withId(CompositionId id);
    CompositionDraftBuilder& withStyleProfile(StyleProfile profile);
    CompositionDraftBuilder& withProjectKey(std::string key);
    CompositionDraftBuilder& withDescription(std::string desc);
    CompositionDraftBuilder& addBlockRef(const BlockId& blockId, Version version, Params localParams = {});
    CompositionDraftBuilder& addStaticText(std::string text);
    CompositionDraftBuilder& addSeparator(SeparatorType type);

    [[nodiscard]] Composition build() const;

private:
    Composition comp_;
};

} // namespace tf
