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
    RenderContext& withParam(const std::string& name, ParamValue value) {
        params[name] = std::move(value);
        return *this;
    }

    /**
     * Set target language
     */
    RenderContext& withLanguage(std::string lang) {
        targetLanguage = std::move(lang);
        return *this;
    }

    /**
     * Set strict mode
     */
    RenderContext& withStrictMode(bool strict) {
        strictMode = strict;
        return *this;
    }
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
    std::optional<std::string> blockWrapper;  ///< Template with {+{content}+} placeholder
    std::optional<std::string> preamble;      ///< Text before composition
    std::optional<std::string> postamble;     ///< Text after composition
    std::optional<std::string> delimiter;     ///< Separator between fragments

    [[nodiscard]] static StructuralStyle plain() {
        return StructuralStyle{};
    }

    [[nodiscard]] static StructuralStyle markdown() {
        StructuralStyle style;
        style.outputFormat = OutputFormat::Markdown;
        return style;
    }

    [[nodiscard]] static StructuralStyle json() {
        StructuralStyle style;
        style.outputFormat = OutputFormat::Json;
        return style;
    }
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

    [[nodiscard]] bool isEmpty() const noexcept {
        return !tone.has_value() &&
               !tense.has_value() &&
               !targetLanguage.has_value() &&
               !person.has_value();
    }
};

/**
 * StyleProfile - combined structural and semantic styles
 */
struct StyleProfile {
    StructuralStyle structural;
    SemanticStyle semantic;

    [[nodiscard]] static StyleProfile plain() {
        return StyleProfile{};
    }

    [[nodiscard]] static StyleProfile markdown() {
        StyleProfile profile;
        profile.structural = StructuralStyle::markdown();
        return profile;
    }
};

/**
 * Composition - ordered list of fragments that forms a complete text
 * Consists of BlockRef, StaticText, and Separator fragments
 */
class Composition {
public:
    Composition() = default;
    explicit Composition(CompositionId id) : id_(std::move(id)) {}

    // Getters
    [[nodiscard]] const CompositionId& id() const noexcept { return id_; }
    [[nodiscard]] BlockState state() const noexcept { return state_; }
    [[nodiscard]] const Version& version() const noexcept { return version_; }
    [[nodiscard]] const std::vector<Fragment>& fragments() const noexcept { return fragments_; }
    [[nodiscard]] const std::optional<StyleProfile>& styleProfile() const noexcept { return styleProfile_; }
    [[nodiscard]] const std::string& projectKey() const noexcept { return projectKey_; }
    [[nodiscard]] const std::string& description() const noexcept { return description_; }

    // Setters (allowed only for Draft state)
    void setId(CompositionId id) { id_ = std::move(id); }
    void setStyleProfile(StyleProfile profile) { styleProfile_ = std::move(profile); }
    void setProjectKey(std::string key) { projectKey_ = std::move(key); }
    void setDescription(std::string desc) { description_ = std::move(desc); }

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
    [[nodiscard]] Fragment& fragment(size_t index) { return fragments_.at(index); }
    [[nodiscard]] const Fragment& fragment(size_t index) const { return fragments_.at(index); }

    [[nodiscard]] size_t fragmentCount() const noexcept { return fragments_.size(); }

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
    CompositionDraftBuilder() = default;
    explicit CompositionDraftBuilder(CompositionId id) : comp_(std::move(id)) {}

    CompositionDraftBuilder& withId(CompositionId id) {
        comp_.setId(std::move(id));
        return *this;
    }

    CompositionDraftBuilder& withStyleProfile(StyleProfile profile) {
        comp_.setStyleProfile(std::move(profile));
        return *this;
    }

    CompositionDraftBuilder& withProjectKey(std::string key) {
        comp_.setProjectKey(std::move(key));
        return *this;
    }

    CompositionDraftBuilder& withDescription(std::string desc) {
        comp_.setDescription(std::move(desc));
        return *this;
    }

    CompositionDraftBuilder& addBlockRef(const BlockId& blockId, Version version, Params localParams = {}) {
        comp_.addBlockRef(blockId, version, std::move(localParams));
        return *this;
    }

    CompositionDraftBuilder& addStaticText(std::string text) {
        comp_.addStaticText(std::move(text));
        return *this;
    }

    CompositionDraftBuilder& addSeparator(SeparatorType type) {
        comp_.addSeparator(type);
        return *this;
    }

    [[nodiscard]] Composition build() const {
        return comp_;
    }

private:
    Composition comp_;
};

} // namespace tf
