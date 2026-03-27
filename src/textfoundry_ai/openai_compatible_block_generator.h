#pragma once

#include <map>
#include <optional>
#include <string>

#include "tf/block_generation.h"

namespace tf::ai {

struct HttpRequest {
  std::string url;
  std::map<std::string, std::string> headers;
  std::string body;
};

struct HttpResponse {
  int status_code = 0;
  std::string body;
};

class IHttpTransport {
 public:
  virtual ~IHttpTransport() = default;

  [[nodiscard]] virtual Result<HttpResponse> PostJson(
      const HttpRequest& request) const = 0;
};

struct OpenAiCompatibleConfig {
  std::string base_url;
  std::string model;
  std::string api_key;
  std::optional<std::string> organization;
  std::string endpoint_path = "/chat/completions";
};

/**
 * OpenAI-compatible block generator adapter.
 *
 * The HTTP transport is intentionally kept out of the core engine. This module
 * owns provider-specific configuration and request/response mapping.
 */
class OpenAiCompatibleBlockGenerator final : public IBlockGenerator {
 public:
  OpenAiCompatibleBlockGenerator(OpenAiCompatibleConfig config,
                                 std::shared_ptr<IHttpTransport> transport);

  [[nodiscard]] const OpenAiCompatibleConfig& config() const noexcept {
    return config_;
  }

  [[nodiscard]] Result<GeneratedBlockData> GenerateBlock(
      const BlockGenerationRequest& request) const override;

  [[nodiscard]] Result<GeneratedBlockBatch> GenerateBlocks(
      const PromptSlicingRequest& request) const override;

 private:
  [[nodiscard]] Error ValidateConfig() const;
  [[nodiscard]] Result<HttpRequest> BuildRequest(
      const BlockGenerationRequest& request) const;
  [[nodiscard]] Result<HttpRequest> BuildBatchRequest(
      const PromptSlicingRequest& request) const;
  [[nodiscard]] Result<GeneratedBlockData> ParseResponse(
      const HttpResponse& response) const;
  [[nodiscard]] Result<GeneratedBlockBatch> ParseBatchResponse(
      const HttpResponse& response) const;

  OpenAiCompatibleConfig config_;
  std::shared_ptr<IHttpTransport> transport_;
};

}  // namespace tf::ai
