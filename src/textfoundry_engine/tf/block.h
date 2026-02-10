//
// Created by artem.d on 28.01.2026.
//

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "block_type.hpp"
#include "error.h"
#include "types.h"
#include "version.h"

namespace tf {
class BlockRef;  // Forward declaration

/**
 * Template with placeholders like "Hello, {{name}}!"
 */
class Template {
 public:
  Template() = default;

  explicit Template(std::string content) : content_(std::move(content)) {}

  [[nodiscard]] const std::string& Content() const noexcept { return content_; }
  void SetContent(std::string content) { content_ = std::move(content); }

  /**
   * Extract parameter names from template placeholders {{paramName}}
   */
  [[nodiscard]] std::vector<std::string> ExtractParamNames() const;

  /**
   * Expand template with provided parameters
   * @returns expanded string or Error if parameters missing
   */
  [[nodiscard]] Result<std::string> Expand(const Params& params) const;

 private:
  std::string content_;
};

/**
 * Block - reusable logical text fragment
 * Block is deterministic, contains no logic, doesn't depend on execution
 * context All parameters are required unless default value is specified
 */
class Block {
 public:
  // Construction
  Block() = default;

  explicit Block(BlockId id) : id_(std::move(id)) {}

  // Getters
  [[nodiscard]] const BlockId& Id() const noexcept;

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
  void SetId(BlockId id);

  void SetType(BlockType type);

  void SetState(BlockState state);

  void SetTemplate(Template templ);

  void SetDefaults(Params defaults);

  void SetParamSchema(std::vector<ParamSchema> schema);

  void SetTags(std::unordered_set<std::string> tags);

  void SetLanguage(std::string lang);

  void SetDescription(std::string desc);

  void SetVersion(const Version& v);

  /**
   * Resolve parameter value using hierarchy:
   * 1. Block Defaults (lowest priority)
   * 2. Composition Local Override
   * 3. Runtime Context (highest priority)
   */
  [[nodiscard]] Result<ParamValue> ResolveParam(
      const std::string& param_name, const Params& local_override,
      const Params& runtime_context) const;

  /**
   * Validate that all required parameters can be resolved
   */
  [[nodiscard]] Error ValidateParams(const Params& local_override,
                                     const Params& runtime_context) const;

  /**
   * Publish this block - creates immutable version
   * @param new_version version to assign
   * @returns Error if block cannot be published (e.g., not in Draft state)
   */
  [[nodiscard]] Error publish(const Version& new_version);

  /**
   * Mark this version as deprecated
   */
  void deprecate();

  /**
   * Check if parameter has default value or is provided in overrides
   */
  [[nodiscard]] bool CanResolveParam(const std::string& param_name,
                                     const Params& local_override,
                                     const Params& runtime_context) const;

 private:
  // Identity
  BlockId id_;
  BlockType type_ = BlockType::Domain;
  BlockState state_ = BlockState::Draft;
  Version version_{0, 0};

  // Content
  Template template_;
  Params defaults_;  ///< Default parameter values
  std::vector<ParamSchema>
      paramSchema_;  ///< Parameter schema with required flags

  // Metadata
  std::unordered_set<std::string> tags_;
  std::string language_ = "en";
  std::string description_;
};

class BlockDraft {
  friend class Engine;
  friend class BlockDraftBuilder;

 public:
  BlockDraft(BlockDraft&&) = default;

 private:
  Block internal_;

  explicit BlockDraft(Block&& b) : internal_(std::move(b)) {}
};

class PublishedBlock {
  friend class Engine;

 public:
  BlockId id() const noexcept { return id_; }
  Version version() const noexcept { return version_; }

  [[nodiscard]] BlockRef ref() const;

 private:
  BlockId id_;
  Version version_;

  explicit PublishedBlock(BlockId id, Version ver)
      : id_(std::move(id)), version_(ver) {}
};

/**
 * Builder for creating Block drafts
 */
class BlockDraftBuilder {
 public:
  explicit BlockDraftBuilder(BlockId id);

  BlockDraftBuilder& WithId(BlockId id);

  BlockDraftBuilder& WithType(const BlockType& type);

  BlockDraftBuilder& WithTemplate(Template templ);

  BlockDraftBuilder& WithDefault(const std::string& name, ParamValue value);

  BlockDraftBuilder& WithDefaults(Params defaults);

  BlockDraftBuilder& WithTag(const std::string& tag);

  BlockDraftBuilder& WithLanguage(std::string lang);

  BlockDraftBuilder& WithDescription(std::string desc);

  [[nodiscard]] BlockDraft build();

 private:
  Block block_;
};
}  // namespace tf
