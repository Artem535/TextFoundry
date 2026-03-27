//
// Created by a.durynin on 29.01.2026.
//

#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

namespace tf {
/**
 * Taxonomic category of a block.
 * Type is metadata (tag), not a class/interface defining behavior.
 * Does NOT affect Template Expansion - Renderer processes all types
 * identically. Used only for filtering, validation, and UI organization.
 */
enum class BlockType {
  Role,        ///< Role/instructions for LLM
  System,      ///< System-level instructions and framing
  Mission,     ///< Goals, tasks, and objectives
  Safety,      ///< Safety rules and guardrails
  Constraint,  ///< Numeric constraints, requirements
  Style,       ///< Stylistic parameters
  Domain,      ///< Domain-specific blocks
  Meta         ///< Metadata, utility blocks
};

/**
 * Convert BlockType to string representation
 */
constexpr std::string_view BlockTypeToString(const BlockType& type) noexcept {
  switch (type) {
    case BlockType::Role:
      return "role";
    case BlockType::System:
      return "system";
    case BlockType::Mission:
      return "mission";
    case BlockType::Safety:
      return "safety";
    case BlockType::Constraint:
      return "constraint";
    case BlockType::Style:
      return "style";
    case BlockType::Domain:
      return "domain";
    case BlockType::Meta:
      return "meta";
  }
  return "unknown";
}

/**
 * Parse BlockType from string
 * @throws std::invalid_argument if string is not recognized
 */
inline BlockType BlockTypeFromString(const std::string_view& str) {
  if (str == "role") return BlockType::Role;
  if (str == "system") return BlockType::System;
  if (str == "mission") return BlockType::Mission;
  if (str == "safety") return BlockType::Safety;
  if (str == "constraint") return BlockType::Constraint;
  if (str == "style") return BlockType::Style;
  if (str == "domain") return BlockType::Domain;
  if (str == "meta") return BlockType::Meta;
  throw std::invalid_argument("Unknown BlockType: " + std::string(str));
}

/**
 * Lifecycle state of an entity
 */
enum class BlockState {
  Draft,      ///< Editable version
  Published,  ///< Fixed immutable version
  Deprecated  ///< Outdated but readable
};

/**
 * Convert BlockState to string representation
 */
constexpr std::string_view BlockStateToString(
    const BlockState& state) noexcept {
  switch (state) {
    case BlockState::Draft:
      return "draft";
    case BlockState::Published:
      return "published";
    case BlockState::Deprecated:
      return "deprecated";
  }
  return "unknown";
}

/**
 * Separator type in composition
 */
enum class SeparatorType {
  Newline,    ///< Line break
  Paragraph,  ///< Empty line (paragraph break)
  Hr          ///< Horizontal rule
};

/**
 * Convert SeparatorType to string representation
 */
constexpr std::string_view SeparatorTypeToString(
    const SeparatorType& type) noexcept {
  switch (type) {
    case SeparatorType::Newline:
      return "newline";
    case SeparatorType::Paragraph:
      return "paragraph";
    case SeparatorType::Hr:
      return "hr";
  }
  return "unknown";
}
}  // namespace tf
