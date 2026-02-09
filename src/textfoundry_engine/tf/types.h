//
// Created by artem.d on 01.02.2026.
//

#pragma once
#include <optional>
#include <string>
#include <unordered_map>

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
 * BlockId - globally unique identifier using dot notation
 * (domain.subdomain.name) First segment often matches BlockType, but this is
 * convention, not requirement
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
}  // namespace tf