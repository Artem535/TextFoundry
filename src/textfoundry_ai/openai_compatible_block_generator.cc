#include "openai_compatible_block_generator.h"

#include <utility>

namespace tf::ai {

OpenAiCompatibleBlockGenerator::OpenAiCompatibleBlockGenerator(
    OpenAiCompatibleConfig config)
    : config_(std::move(config)) {}

Result<GeneratedBlockData> OpenAiCompatibleBlockGenerator::GenerateBlock(
    const BlockGenerationRequest& request) const {
  if (auto err = ValidateConfig(); err.is_error()) {
    return Result<GeneratedBlockData>(std::move(err));
  }
  if (request.prompt.empty()) {
    return Result<GeneratedBlockData>(
        Error{ErrorCode::InvalidParamType, "Block generation prompt is empty"});
  }

  return Result<GeneratedBlockData>(Error{
      ErrorCode::StorageError,
      "OpenAI-compatible block generation transport is not implemented yet"});
}

Error OpenAiCompatibleBlockGenerator::ValidateConfig() const {
  if (config_.base_url.empty()) {
    return Error{ErrorCode::InvalidParamType,
                 "OpenAI-compatible base_url is required"};
  }
  if (config_.model.empty()) {
    return Error{ErrorCode::InvalidParamType,
                 "OpenAI-compatible model is required"};
  }
  if (config_.api_key.empty()) {
    return Error{ErrorCode::InvalidParamType,
                 "OpenAI-compatible api_key is required"};
  }

  return Error::success();
}

}  // namespace tf::ai
