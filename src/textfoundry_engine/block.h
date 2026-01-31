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
    [[nodiscard]] std::vector<std::string> extract_param_names() const;

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
    [[nodiscard]] const BlockId& id() const noexcept;

    [[nodiscard]] BlockType type() const noexcept;

    [[nodiscard]] BlockState state() const noexcept;

    [[nodiscard]] const Version& version() const noexcept;

    [[nodiscard]] const Template& templ() const noexcept;

    [[nodiscard]] const Params& defaults() const noexcept;

    [[nodiscard]] const std::vector<ParamSchema>& param_schema() const noexcept;

    [[nodiscard]] const std::unordered_set<std::string>& tags() const noexcept;

    [[nodiscard]] const std::string& language() const noexcept;

    [[nodiscard]] const std::string& description() const noexcept;

    // Setters (allowed only for Draft state)
    void set_id(BlockId id);

    void set_type(BlockType type);

    void set_state(BlockState state);

    void set_template(Template templ);

    void set_defaults(Params defaults);

    void set_param_schema(std::vector<ParamSchema> schema);

    void set_tags(std::unordered_set<std::string> tags);

    void set_language(std::string lang);

    void set_description(std::string desc);

    void set_version(const Version& v);

    /**
     * Resolve parameter value using hierarchy:
     * 1. Block Defaults (lowest priority)
     * 2. Composition Local Override
     * 3. Runtime Context (highest priority)
     */
    [[nodiscard]] Result<ParamValue> resolve_param(
        const std::string& param_name,
        const Params& local_override,
        const Params& runtime_context
    ) const;

    /**
     * Validate that all required parameters can be resolved
     */
    [[nodiscard]] Error validate_params(
        const Params& local_override,
        const Params& runtime_context
    ) const;

    /**
     * Publish this block - creates immutable version
     * @param new_version version to assign
     * @returns Error if block cannot be published (e.g., not in Draft state)
     */
    [[nodiscard]] Error publish(const Version &new_version);

    /**
     * Mark this version as deprecated
     */
    void deprecate();

    /**
     * Check if parameter has default value or is provided in overrides
     */
    [[nodiscard]] bool can_resolve_param(
        const std::string& param_name,
        const Params& local_override,
        const Params& runtime_context
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
    explicit BlockDraftBuilder(BlockId id);

    BlockDraftBuilder& with_id(BlockId id);
    BlockDraftBuilder& with_type(const BlockType &type);
    BlockDraftBuilder& with_template(Template templ);
    BlockDraftBuilder& with_default(const std::string& name, ParamValue value);
    BlockDraftBuilder& with_defaults(Params defaults);
    BlockDraftBuilder& with_tag(const std::string& tag);
    BlockDraftBuilder& with_language(std::string lang);
    BlockDraftBuilder& with_description(std::string desc);

    [[nodiscard]] Block build() const;

private:
    Block block_;
};

} // namespace tf
