#pragma once

#include <chrono>

#include "openai_compatible_block_generator.h"

namespace tf::ai {

class QtHttpTransport final : public IHttpTransport {
 public:
  explicit QtHttpTransport(
      std::chrono::milliseconds timeout = std::chrono::seconds(30));

  [[nodiscard]] Result<HttpResponse> PostJson(
      const HttpRequest& request) const override;

 private:
  std::chrono::milliseconds timeout_;
};

}  // namespace tf::ai
