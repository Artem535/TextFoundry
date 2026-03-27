#pragma once

#include "openai_compatible_block_generator.h"
#include "tf/engine.h"

namespace tf::ai {

class OpenAiCompatibleNormalizer final : public INormalizer,
                                         public IBlockNormalizer {
 public:
  OpenAiCompatibleNormalizer(OpenAiCompatibleConfig config,
                             std::shared_ptr<IHttpTransport> transport);

  [[nodiscard]] Result<std::string> Normalize(
      const std::string& text, const SemanticStyle& style) const override;

  [[nodiscard]] Result<NormalizedBlockData> NormalizeBlock(
      const BlockNormalizationRequest& request) const override;

  [[nodiscard]] std::string Fingerprint() const override;

 private:
  [[nodiscard]] Error ValidateConfig() const;
  [[nodiscard]] Result<HttpRequest> BuildRequest(
      const std::string& text, const SemanticStyle& style) const;
  [[nodiscard]] Result<HttpRequest> BuildBlockRequest(
      const BlockNormalizationRequest& request) const;
  [[nodiscard]] Result<std::string> ParseResponse(
      const HttpResponse& response) const;

  OpenAiCompatibleConfig config_;
  std::shared_ptr<IHttpTransport> transport_;
};

}  // namespace tf::ai
