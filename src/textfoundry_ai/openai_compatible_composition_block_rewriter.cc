#include "openai_compatible_composition_block_rewriter.h"

#include "prompt_constants.h"

#include <format>
#include <rfl/json.hpp>
#include <sstream>

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

  struct PatchPropertiesSchema {
    StringSchema block_id;
    StringSchema description;
    StringSchema templ;
    StringMapSchema defaults;
    StringArraySchema tags;
    StringSchema rationale;
  };

  struct PatchSchema {
    std::string type = "object";
    bool additionalProperties = false;
    std::vector<std::string> required = {"block_id", "rationale"};
    PatchPropertiesSchema properties;
  };

  struct PatchesArraySchema {
    std::string type = "array";
    PatchSchema items;
  };

  struct RootPropertiesSchema {
    PatchesArraySchema patches;
  };

  struct SchemaRoot {
    std::string type = "object";
    bool additionalProperties = false;
    std::vector<std::string> required = {"patches"};
    RootPropertiesSchema properties;
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

struct RewritePatchPayload {
  std::string block_id;
  std::optional<std::string> description;
  std::optional<std::string> templ;
  std::optional<Params> defaults;
  std::optional<std::vector<std::string>> tags;
  std::string rationale;
};

struct RewritePreviewPayload {
  std::vector<RewritePatchPayload> patches;
};

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

std::string BuildSystemPrompt() {
  return std::string(prompts::kCompositionRewriteSystemPrompt);
}

std::string BuildUserPrompt(const CompositionBlockRewriteContext& request) {
  std::string prompt = std::string(prompts::kCompositionRewriteUserIntro);
  prompt += std::string(prompts::kCompositionRewriteInstructionLabel);
  prompt += request.instruction;
  prompt += std::string(prompts::kCompositionRewriteCompositionLabel);
  prompt += "Composition id: " + request.source_composition_id + "\n";
  prompt += "Version: " + std::to_string(request.source_version.major) + "." +
            std::to_string(request.source_version.minor) + "\n";
  prompt += std::string(prompts::kCompositionRewriteConstraints);

  for (const auto& block : request.blocks) {
    prompt += "\n---\n";
    prompt += "Block id: " + block.block_id + "\n";
    prompt += "Type: " + std::string(BlockTypeToString(block.type)) + "\n";
    prompt += "Language: " + block.language + "\n";
    prompt += "Description:\n" + block.description + "\n";
    prompt += "Tags:\n";
    for (const auto& tag : block.tags) {
      prompt += "- " + tag + "\n";
    }
    prompt += "Defaults:\n";
    for (const auto& [key, value] : block.defaults) {
      prompt += "- " + key + "=" + value + "\n";
    }
    prompt += "Template:\n" + block.templ + "\n";
  }

  return prompt;
}

}  // namespace

OpenAiCompatibleCompositionBlockRewriter::
    OpenAiCompatibleCompositionBlockRewriter(
        OpenAiCompatibleConfig config,
        std::shared_ptr<IHttpTransport> transport)
    : config_(std::move(config)), transport_(std::move(transport)) {}

Result<CompositionBlockRewritePreview>
OpenAiCompatibleCompositionBlockRewriter::PreviewRewrite(
    const CompositionBlockRewriteContext& request) const {
  if (auto err = ValidateConfig(); err.is_error()) {
    return Result<CompositionBlockRewritePreview>(std::move(err));
  }
  if (!transport_) {
    return Result<CompositionBlockRewritePreview>(
        Error{ErrorCode::StorageError,
              "OpenAI-compatible transport is not configured"});
  }
  if (request.instruction.empty()) {
    return Result<CompositionBlockRewritePreview>(
        Error{ErrorCode::InvalidParamType, "Rewrite instruction is empty"});
  }
  if (request.blocks.empty()) {
    return Result<CompositionBlockRewritePreview>(
        Error{ErrorCode::InvalidParamType, "Rewrite context has no blocks"});
  }

  auto http_request = BuildRequest(request);
  if (http_request.HasError()) {
    return Result<CompositionBlockRewritePreview>(http_request.error());
  }

  auto response = transport_->PostJson(http_request.value());
  if (response.HasError()) {
    return Result<CompositionBlockRewritePreview>(response.error());
  }

  return ParseResponse(response.value());
}

std::string OpenAiCompatibleCompositionBlockRewriter::Fingerprint() const {
  std::ostringstream stream;
  stream << "openai-compatible|"
         << config_.base_url << "|" << config_.model
         << "|composition-block-rewriter-v1";
  return stream.str();
}

Error OpenAiCompatibleCompositionBlockRewriter::ValidateConfig() const {
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

Result<HttpRequest> OpenAiCompatibleCompositionBlockRewriter::BuildRequest(
    const CompositionBlockRewriteContext& request) const {
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
                      .name = "textfoundry_composition_block_rewrite",
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

Result<CompositionBlockRewritePreview>
OpenAiCompatibleCompositionBlockRewriter::ParseResponse(
    const HttpResponse& response) const {
  if (response.status_code < 200 || response.status_code >= 300) {
    return Result<CompositionBlockRewritePreview>(
        Error{ErrorCode::StorageError,
              std::format(
                  "OpenAI-compatible composition rewrite request failed with status {}",
                  response.status_code)});
  }

  const auto parsed = rfl::json::read<OpenAiChatCompletionResponse>(response.body);
  if (!parsed) {
    return Result<CompositionBlockRewritePreview>(
        Error{ErrorCode::InvalidParamType,
              "Failed to parse OpenAI-compatible composition rewrite response JSON"});
  }
  if (parsed->choices.empty()) {
    return Result<CompositionBlockRewritePreview>(
        Error{ErrorCode::InvalidParamType,
              "OpenAI-compatible composition rewrite response has no choices"});
  }

  const auto& message = parsed->choices.front().message;
  if (!message.content.has_value() || message.content->empty()) {
    return Result<CompositionBlockRewritePreview>(Error{
        ErrorCode::InvalidParamType,
        "OpenAI-compatible composition rewrite response does not contain message content"});
  }

  const auto payload =
      rfl::json::read<RewritePreviewPayload>(*message.content);
  if (!payload) {
    return Result<CompositionBlockRewritePreview>(
        Error{ErrorCode::InvalidParamType,
              "Failed to parse composition block rewrite patch payload"});
  }

  CompositionBlockRewritePreview preview;
  preview.patches.reserve(payload->patches.size());
  for (const auto& patch : payload->patches) {
    preview.patches.push_back(BlockRewritePatch{
        .block_id = patch.block_id,
        .description = patch.description,
        .templ = patch.templ,
        .defaults = patch.defaults,
        .tags = patch.tags,
        .rationale = patch.rationale,
    });
  }

  return Result<CompositionBlockRewritePreview>(std::move(preview));
}

}  // namespace tf::ai
