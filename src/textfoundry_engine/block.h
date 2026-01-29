//
// Created by artem.d on 28.01.2026.
//

#pragma once

#include "blocktype.hpp"
#include "version.h"
#include "error.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>

namespace tf {

/**
 * Type alias for parameter values
 */
using ParamValue = std::string;

/**
 * Type alias for parameter map
 */
using Params = std::unordered_map<std::string, ParamValue>;

/**
 * BlockId - globally unique identifier using dot notation (domain.subdomain.name)
 * First segment often matches BlockType, but this is convention, not requirement
 */
using BlockId = std::string;

/**
 * Schema for a single parameter
 */
struct ParamSchema {
    std::string name;
    bool required = false;
    std::optional<ParamValue> defaultValue;
};

/**
 * Template with placeholders like "Hello, {{name}}!"
 */
class Template {
public:
    Template() = default;
    explicit Template(std::string content) : content_(std::move(content)) {}

    [[nodiscard]] const std::string& content() const noexcept { return content_; }
    void setContent(std::string content) { content_ = std::move(content); }

    /**
     * Extract parameter names from template placeholders {{paramName}}
     */
    [[nodiscard]] std::vector<std::string> extractParamNames() const;

    /**
     * Expand template with provided parameters
     * @returns expanded string or Error if parameters missing
     */
    [[nodiscard]] Result<std::string> expand(const Params& params) const;

private:
    std::string content_;
};

/**
 * Block - reusable logical text fragment
 * Block is deterministic, contains no logic, doesn't depend on execution context
 * All parameters are required unless default value is specified
 */
class Block {
public:
    // Construction
    Block() = default;
    explicit Block(BlockId id) : id_(std::move(id)) {}

    // Getters
    [[nodiscard]] const BlockId& id() const noexcept { return id_; }
    [[nodiscard]] BlockType type() const noexcept { return type_; }
    [[nodiscard]] BlockState state() const noexcept { return state_; }
    [[nodiscard]] const Version& version() const noexcept { return version_; }
    [[nodiscard]] const Template& templ() const noexcept { return template_; }
    [[nodiscard]] const Params& defaults() const noexcept { return defaults_; }
    [[nodiscard]] const std::vector<ParamSchema>& paramSchema() const noexcept { return paramSchema_; }
    [[nodiscard]] const std::unordered_set<std::string>& tags() const noexcept { return tags_; }
    [[nodiscard]] const std::string& language() const noexcept { return language_; }
    [[nodiscard]] const std::string& description() const noexcept { return description_; }

    // Setters (allowed only for Draft state)
    void setId(BlockId id) { id_ = std::move(id); }
    void setType(BlockType type) { type_ = type; }
    void setTemplate(Template templ) { template_ = std::move(templ); }
    void setDefaults(Params defaults) { defaults_ = std::move(defaults); }
    void setParamSchema(std::vector<ParamSchema> schema) { paramSchema_ = std::move(schema); }
    void setTags(std::unordered_set<std::string> tags) { tags_ = std::move(tags); }
    void setLanguage(std::string lang) { language_ = std::move(lang); }
    void setDescription(std::string desc) { description_ = std::move(desc); }

    /**
     * Resolve parameter value using hierarchy:
     * 1. Block Defaults (lowest priority)
     * 2. Composition Local Override
     * 3. Runtime Context (highest priority)
     */
    [[nodiscard]] Result<ParamValue> resolveParam(
        const std::string& paramName,
        const Params& localOverride,
        const Params& runtimeContext
    ) const;

    /**
     * Validate that all required parameters can be resolved
     */
    [[nodiscard]] Error validateParams(
        const Params& localOverride,
        const Params& runtimeContext
    ) const;

    /**
     * Publish this block - creates immutable version
     * @param newVersion version to assign
     * @returns Error if block cannot be published (e.g., not in Draft state)
     */
    [[nodiscard]] Error publish(Version newVersion);

    /**
     * Mark this version as deprecated
     */
    void deprecate();

    /**
     * Check if parameter has default value or is provided in overrides
     */
    [[nodiscard]] bool canResolveParam(
        const std::string& paramName,
        const Params& localOverride,
        const Params& runtimeContext
    ) const;

private:
    // Identity
    BlockId id_;
    BlockType type_ = BlockType::Domain;
    BlockState state_ = BlockState::Draft;
    Version version_{0, 0};

    // Content
    Template template_;
    Params defaults_;  ///< Default parameter values
    std::vector<ParamSchema> paramSchema_;  ///< Parameter schema with required flags

    // Metadata
    std::unordered_set<std::string> tags_;
    std::string language_ = "en";
    std::string description_;
};

/**
 * Builder for creating Block drafts
 */
class BlockDraftBuilder {
public:
    BlockDraftBuilder() = default;
    explicit BlockDraftBuilder(BlockId id) : block_(std::move(id)) {}

    BlockDraftBuilder& withId(BlockId id) {
        block_.setId(std::move(id));
        return *this;
    }

    BlockDraftBuilder& withType(BlockType type) {
        block_.setType(type);
        return *this;
    }

    BlockDraftBuilder& withTemplate(Template templ) {
        block_.setTemplate(std::move(templ));
        return *this;
    }

    BlockDraftBuilder& withDefault(const std::string& name, ParamValue value) {
        auto defaults = block_.defaults();
        defaults[name] = std::move(value);
        block_.setDefaults(std::move(defaults));
        return *this;
    }

    BlockDraftBuilder& withDefaults(Params defaults) {
        block_.setDefaults(std::move(defaults));
        return *this;
    }

    BlockDraftBuilder& withTag(const std::string& tag) {
        auto tags = block_.tags();
        tags.insert(tag);
        block_.setTags(std::move(tags));
        return *this;
    }

    BlockDraftBuilder& withLanguage(std::string lang) {
        block_.setLanguage(std::move(lang));
        return *this;
    }

    BlockDraftBuilder& withDescription(std::string desc) {
        block_.setDescription(std::move(desc));
        return *this;
    }

    [[nodiscard]] Block build() const {
        return block_;
    }

private:
    Block block_;
};

} // namespace tf
