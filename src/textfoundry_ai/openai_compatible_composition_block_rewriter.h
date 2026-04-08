#pragma once

#include "openai_compatible_block_generator.h"
#include "tf/engine.h"

namespace tf::ai {

class OpenAiCompatibleCompositionBlockRewriter final
    : public ICompositionBlockRewriter {
 public:
  OpenAiCompatibleCompositionBlockRewriter(
      OpenAiCompatibleConfig config,
      std::shared_ptr<IHttpTransport> transport);

  [[nodiscard]] Result<CompositionBlockRewritePreview> PreviewRewrite(
      const CompositionBlockRewriteContext& request) const override;

  [[nodiscard]] std::string Fingerprint() const override;

 private:
  [[nodiscard]] Error ValidateConfig() const;
  [[nodiscard]] Result<HttpRequest> BuildRequest(
      const CompositionBlockRewriteContext& request) const;
  [[nodiscard]] Result<CompositionBlockRewritePreview> ParseResponse(
      const HttpResponse& response) const;

  OpenAiCompatibleConfig config_;
  std::shared_ptr<IHttpTransport> transport_;
};

}  // namespace tf::ai
