#include "openai_compatible_block_generator.h"

#include <format>
#include <optional>
#include <rfl/json.hpp>
#include <utility>

#include "prompt_constants.h"
#include "tf/logger.h"

namespace tf::ai {
namespace {

struct OpenAiMessage {
  std::string role;
  std::string content;
};

struct OpenAiJsonSchema {
  std::string name;
  bool strict = true;
  struct StringSchema {
    std::string type = "string";
  };

  struct StringMapSchema {
    std::string type = "object";
    StringSchema additionalProperties;
  };

  struct StringArraySchema {
    std::string type = "array";
    StringSchema items;
  };

  struct BlockPropertiesSchema {
    StringSchema id;
    StringSchema type;
    StringSchema language;
    StringSchema description;
    StringSchema templ;
    StringMapSchema defaults;
    StringArraySchema tags;
  };

  struct SchemaRoot {
    std::string type = "object";
    bool additionalProperties = false;
    std::vector<std::string> required;
    BlockPropertiesSchema properties;
  };

  SchemaRoot schema;
};

struct OpenAiResponseFormat {
  std::string type = "json_schema";
  OpenAiJsonSchema json_schema;
};

struct OpenAiChatCompletionRequest {
  std::string model;
  std::vector<OpenAiMessage> messages;
  OpenAiResponseFormat response_format;
};

struct OpenAiMessageResponse {
  std::optional<std::string> content;
  std::string role;
};

struct OpenAiChoiceResponse {
  OpenAiMessageResponse message;
};

struct OpenAiChatCompletionResponse {
  std::vector<OpenAiChoiceResponse> choices;
};

struct GeneratedBlockPayload {
  std::string id;
  std::string type;
  std::string language = "en";
  std::string description;
  std::string templ;
  Params defaults;
  std::vector<std::string> tags;
};

struct GeneratedBlockBatchPayload {
  std::vector<GeneratedBlockPayload> blocks;
};

OpenAiJsonSchema::SchemaRoot BuildSchema() {
  OpenAiJsonSchema::SchemaRoot schema;
  schema.required = {"id", "type", "language", "description",
                     "templ", "defaults", "tags"};
  return schema;
}

std::string JoinUrl(std::string base_url, const std::string& endpoint_path) {
  if (base_url.empty()) return endpoint_path;
  if (!base_url.empty() && base_url.back() == '/' && !endpoint_path.empty() &&
      endpoint_path.front() == '/') {
    base_url.pop_back();
  } else if (!base_url.empty() && base_url.back() != '/' &&
             !endpoint_path.empty() && endpoint_path.front() != '/') {
    base_url += "/";
  }
  return base_url + endpoint_path;
}

std::string TruncateForLog(std::string_view text, std::size_t max_length = 1200) {
  if (text.size() <= max_length) {
    return std::string(text);
  }
  return std::string(text.substr(0, max_length)) + "... [truncated]";
}

std::string BuildSystemPrompt() {
  return std::string(prompts::kBlockGenerationSystemPrompt);
}

std::string BuildBatchSystemPrompt() {
  return std::string(prompts::kBlockBatchSystemPrompt);
}

std::string BuildUserPrompt(const BlockGenerationRequest& request) {
  std::string prompt = std::string(prompts::kBlockGenerationUserIntro);
  prompt += std::string(prompts::kBlockGenerationUserRequestLabel) +
            request.prompt + "\n";

  if (request.preferred_id.has_value()) {
    prompt += std::string(prompts::kPreferredIdLabel) + *request.preferred_id +
              "\n";
  }
  if (request.preferred_type.has_value()) {
    prompt += std::string(prompts::kPreferredTypeLabel) +
              std::string(BlockTypeToString(*request.preferred_type)) + "\n";
  }
  if (request.preferred_language.has_value()) {
    prompt += std::string(prompts::kPreferredLanguageLabel) +
              *request.preferred_language + "\n";
  }
  if (!request.existing_block_ids.empty()) {
    prompt += std::string(prompts::kExistingBlockIdsToAvoidLabel);
    for (const auto& id : request.existing_block_ids) {
      prompt += "- " + id + "\n";
    }
  }

  prompt += std::string(prompts::kBlockGenerationUserOutro);
  return prompt;
}

std::string BuildBatchUserPrompt(const PromptSlicingRequest& request) {
  std::string prompt = std::string(prompts::kBatchUserIntro);
  prompt += std::string(prompts::kBatchSourceTextLabel) + request.source_text +
            "\n";

  if (request.namespace_prefix.has_value() &&
      !request.namespace_prefix->empty()) {
    prompt += std::string(prompts::kNamespacePrefixLabel) +
              *request.namespace_prefix + "\n";
  }
  if (request.preferred_language.has_value() &&
      !request.preferred_language->empty()) {
    prompt += std::string(prompts::kPreferredLanguageLabel) +
              *request.preferred_language + "\n";
  }
  if (!request.existing_block_ids.empty()) {
    prompt += std::string(prompts::kExistingBlockIdsToAvoidLabel);
    for (const auto& id : request.existing_block_ids) {
      prompt += "- " + id + "\n";
    }
  }
  if (!request.reusable_block_ids.empty()) {
    prompt += std::string(prompts::kReusableIdsLabel);
    for (const auto& id : request.reusable_block_ids) {
      prompt += "- " + id + "\n";
    }
    prompt += std::string(prompts::kReusableIdsGuidance);
    prompt += std::string(prompts::kPreserveStrengthPrefix) +
              std::to_string(request.preserve_reuse_percent) +
              std::string(prompts::kPreserveStrengthSuffix);
    if (request.preserve_order) {
      prompt += std::string(prompts::kPreserveOrderGuidance);
    }
    if (!request.reusable_block_summaries.empty()) {
      prompt += std::string(prompts::kCurrentStructureLabel);
      for (const auto& summary : request.reusable_block_summaries) {
        prompt += "- " + summary + "\n";
      }
    }
  }

  prompt += std::string(prompts::kBatchUserOutro);
  return prompt;
}

Result<GeneratedBlockData> ParseGeneratedPayload(const std::string& content) {
  const auto parsed = rfl::json::read<GeneratedBlockPayload>(content);
  if (!parsed) {
    return Result<GeneratedBlockData>(
        Error{ErrorCode::InvalidParamType,
              "Failed to parse generated block JSON payload"});
  }

  BlockType type;
  try {
    type = BlockTypeFromString(parsed->type);
  } catch (const std::invalid_argument&) {
    return Result<GeneratedBlockData>(Error{
        ErrorCode::InvalidParamType,
        "Generated block type is invalid: " + parsed->type});
  }

  return Result<GeneratedBlockData>(GeneratedBlockData{
      .id = parsed->id,
      .type = type,
      .language = parsed->language,
      .description = parsed->description,
      .templ = parsed->templ,
      .defaults = parsed->defaults,
      .tags = parsed->tags,
  });
}

Result<GeneratedBlockBatch> ParseGeneratedBatchPayload(
    const std::string& content) {
  const auto parsed = rfl::json::read<GeneratedBlockBatchPayload>(content);
  if (!parsed) {
    TF_LOG_WARN("Failed to parse generated block batch payload content: {}",
                TruncateForLog(content));
    return Result<GeneratedBlockBatch>(
        Error{ErrorCode::InvalidParamType,
              "Failed to parse generated block batch JSON payload"});
  }

  std::vector<GeneratedBlockData> blocks;
  blocks.reserve(parsed->blocks.size());
  for (const auto& payload : parsed->blocks) {
    BlockType type;
    try {
      type = BlockTypeFromString(payload.type);
    } catch (const std::invalid_argument&) {
      return Result<GeneratedBlockBatch>(Error{
          ErrorCode::InvalidParamType,
          "Generated block type is invalid: " + payload.type});
    }
    blocks.push_back(GeneratedBlockData{
        .id = payload.id,
        .type = type,
        .language = payload.language,
        .description = payload.description,
        .templ = payload.templ,
        .defaults = payload.defaults,
        .tags = payload.tags,
    });
  }

  return Result<GeneratedBlockBatch>(
      GeneratedBlockBatch{.blocks = std::move(blocks)});
}

}  // namespace

OpenAiCompatibleBlockGenerator::OpenAiCompatibleBlockGenerator(
    OpenAiCompatibleConfig config, std::shared_ptr<IHttpTransport> transport)
    : config_(std::move(config)), transport_(std::move(transport)) {}

Result<GeneratedBlockData> OpenAiCompatibleBlockGenerator::GenerateBlock(
    const BlockGenerationRequest& request) const {
  if (auto err = ValidateConfig(); err.is_error()) {
    return Result<GeneratedBlockData>(std::move(err));
  }
  if (!transport_) {
    return Result<GeneratedBlockData>(
        Error{ErrorCode::StorageError, "OpenAI-compatible transport is not configured"});
  }
  if (request.prompt.empty()) {
    return Result<GeneratedBlockData>(
        Error{ErrorCode::InvalidParamType, "Block generation prompt is empty"});
  }

  auto http_request = BuildRequest(request);
  if (http_request.HasError()) {
    return Result<GeneratedBlockData>(http_request.error());
  }

  auto response = transport_->PostJson(http_request.value());
  if (response.HasError()) {
    return Result<GeneratedBlockData>(response.error());
  }

  return ParseResponse(response.value());
}

Result<GeneratedBlockBatch> OpenAiCompatibleBlockGenerator::GenerateBlocks(
    const PromptSlicingRequest& request) const {
  if (auto err = ValidateConfig(); err.is_error()) {
    return Result<GeneratedBlockBatch>(std::move(err));
  }
  if (!transport_) {
    return Result<GeneratedBlockBatch>(
        Error{ErrorCode::StorageError, "OpenAI-compatible transport is not configured"});
  }
  if (request.source_text.empty()) {
    return Result<GeneratedBlockBatch>(
        Error{ErrorCode::InvalidParamType, "Prompt slicing source text is empty"});
  }

  auto http_request = BuildBatchRequest(request);
  if (http_request.HasError()) {
    return Result<GeneratedBlockBatch>(http_request.error());
  }

  auto response = transport_->PostJson(http_request.value());
  if (response.HasError()) {
    return Result<GeneratedBlockBatch>(response.error());
  }

  return ParseBatchResponse(response.value());
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
  if (config_.endpoint_path.empty()) {
    return Error{ErrorCode::InvalidParamType,
                 "OpenAI-compatible endpoint_path is required"};
  }

  return Error::success();
}

Result<HttpRequest> OpenAiCompatibleBlockGenerator::BuildRequest(
    const BlockGenerationRequest& request) const {
  OpenAiChatCompletionRequest payload{
      .model = config_.model,
      .messages =
          {
              {.role = "system", .content = BuildSystemPrompt()},
              {.role = "user", .content = BuildUserPrompt(request)},
          },
      .response_format =
          {
              .type = "json_schema",
              .json_schema =
                  {
                      .name = "textfoundry_block",
                      .strict = true,
                      .schema = BuildSchema(),
                  },
          },
  };

  const std::string body = rfl::json::write(payload);

  HttpRequest http_request{
      .url = JoinUrl(config_.base_url, config_.endpoint_path),
      .headers =
          {
              {"Authorization", "Bearer " + config_.api_key},
              {"Content-Type", "application/json"},
          },
      .body = std::move(body),
  };

  if (config_.organization.has_value() && !config_.organization->empty()) {
    http_request.headers["OpenAI-Organization"] = *config_.organization;
  }

  return Result<HttpRequest>(std::move(http_request));
}

Result<HttpRequest> OpenAiCompatibleBlockGenerator::BuildBatchRequest(
    const PromptSlicingRequest& request) const {
  struct BlocksArraySchema {
    std::string type = "array";
    OpenAiJsonSchema::SchemaRoot items = BuildSchema();
  };
  struct BatchPropertiesSchema {
    BlocksArraySchema blocks;
  };
  struct BatchSchemaRoot {
    std::string type = "object";
    bool additionalProperties = false;
    std::vector<std::string> required = {"blocks"};
    BatchPropertiesSchema properties;
  };

  struct BatchJsonSchema {
    std::string name;
    bool strict = true;
    BatchSchemaRoot schema;
  };

  struct BatchResponseFormat {
    std::string type = "json_schema";
    BatchJsonSchema json_schema;
  };

  struct BatchRequest {
    std::string model;
    std::vector<OpenAiMessage> messages;
    BatchResponseFormat response_format;
  };

  BatchRequest payload{
      .model = config_.model,
      .messages =
          {
              {.role = "system", .content = BuildBatchSystemPrompt()},
              {.role = "user", .content = BuildBatchUserPrompt(request)},
          },
      .response_format =
          {
              .type = "json_schema",
              .json_schema =
                  {
                      .name = "textfoundry_block_batch",
                      .strict = true,
                      .schema = {},
                  },
          },
  };

  HttpRequest http_request{
      .url = JoinUrl(config_.base_url, config_.endpoint_path),
      .headers =
          {
              {"Authorization", "Bearer " + config_.api_key},
              {"Content-Type", "application/json"},
          },
      .body = rfl::json::write(payload),
  };

  if (config_.organization.has_value() && !config_.organization->empty()) {
    http_request.headers["OpenAI-Organization"] = *config_.organization;
  }

  return Result<HttpRequest>(std::move(http_request));
}

Result<GeneratedBlockData> OpenAiCompatibleBlockGenerator::ParseResponse(
    const HttpResponse& response) const {
  if (response.status_code < 200 || response.status_code >= 300) {
    return Result<GeneratedBlockData>(
        Error{ErrorCode::StorageError,
              std::format("OpenAI-compatible request failed with status {}",
                          response.status_code)});
  }

  const auto parsed = rfl::json::read<OpenAiChatCompletionResponse>(response.body);
  if (!parsed) {
    return Result<GeneratedBlockData>(
        Error{ErrorCode::InvalidParamType,
              "Failed to parse OpenAI-compatible response JSON"});
  }

  if (parsed->choices.empty()) {
    return Result<GeneratedBlockData>(
        Error{ErrorCode::InvalidParamType, "OpenAI-compatible response has no choices"});
  }

  const auto& message = parsed->choices.front().message;
  if (!message.content.has_value() || message.content->empty()) {
    return Result<GeneratedBlockData>(Error{
        ErrorCode::InvalidParamType,
        "OpenAI-compatible response does not contain message content"});
  }

  return ParseGeneratedPayload(*message.content);
}

Result<GeneratedBlockBatch> OpenAiCompatibleBlockGenerator::ParseBatchResponse(
    const HttpResponse& response) const {
  if (response.status_code < 200 || response.status_code >= 300) {
    return Result<GeneratedBlockBatch>(
        Error{ErrorCode::StorageError,
              std::format("OpenAI-compatible request failed with status {}",
                          response.status_code)});
  }

  const auto parsed = rfl::json::read<OpenAiChatCompletionResponse>(response.body);
  if (!parsed) {
    TF_LOG_WARN("Failed to parse OpenAI-compatible batch response body: {}",
                TruncateForLog(response.body));
    return Result<GeneratedBlockBatch>(
        Error{ErrorCode::InvalidParamType,
              "Failed to parse OpenAI-compatible response JSON"});
  }

  if (parsed->choices.empty()) {
    return Result<GeneratedBlockBatch>(
        Error{ErrorCode::InvalidParamType, "OpenAI-compatible response has no choices"});
  }

  const auto& message = parsed->choices.front().message;
  if (!message.content.has_value() || message.content->empty()) {
    TF_LOG_WARN("OpenAI-compatible batch response missing message content. Body: {}",
                TruncateForLog(response.body));
    return Result<GeneratedBlockBatch>(Error{
        ErrorCode::InvalidParamType,
        "OpenAI-compatible response does not contain message content"});
  }

  return ParseGeneratedBatchPayload(*message.content);
}

}  // namespace tf::ai
