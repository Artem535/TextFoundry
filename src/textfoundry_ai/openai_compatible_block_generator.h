#pragma once

#include <optional>
#include <string>

#include "tf/block_generation.h"

namespace tf::ai {

struct OpenAiCompatibleConfig {
  std::string base_url;
  std::string model;
  std::string api_key;
  std::optional<std::string> organization;
};

/**
 * OpenAI-compatible block generator adapter.
 *
 * The HTTP transport is intentionally kept out of the core engine. This module
 * owns provider-specific configuration and request/response mapping.
 */
class OpenAiCompatibleBlockGenerator final : public IBlockGenerator {
 public:
  explicit OpenAiCompatibleBlockGenerator(OpenAiCompatibleConfig config);

  [[nodiscard]] const OpenAiCompatibleConfig& config() const noexcept {
    return config_;
  }

  [[nodiscard]] Result<GeneratedBlockData> GenerateBlock(
      const BlockGenerationRequest& request) const override;

 private:
  [[nodiscard]] Error ValidateConfig() const;

  OpenAiCompatibleConfig config_;
};

}  // namespace tf::ai
