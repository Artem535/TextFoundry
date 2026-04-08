#include "openai_compatible_normalizer.h"

#include "prompt_constants.h"

#include <rfl/json.hpp>
#include <sstream>

namespace tf::ai {
namespace {

struct OpenAiMessage {
  std::string role;
  std::string content;
};

struct OpenAiChatCompletionRequest {
  std::string model;
  std::vector<OpenAiMessage> messages;
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

std::string BuildSystemPrompt() {
  return std::string(prompts::kTextNormalizationSystemPrompt);
}

std::string BuildBlockSystemPrompt() {
  return std::string(prompts::kBlockNormalizationSystemPrompt);
}

void AppendStyleDirectives(std::string& prompt, const SemanticStyle& style) {
  if (style.tone.has_value()) {
    prompt += "- tone: " + *style.tone + "\n";
  }
  if (style.tense.has_value()) {
    prompt += "- tense: " + *style.tense + "\n";
  }
  if (style.targetLanguage.has_value()) {
    prompt += "- target_language: " + *style.targetLanguage + "\n";
  }
  if (style.person.has_value()) {
    prompt += "- person: " + *style.person + "\n";
  }
  if (style.rewriteStrength.has_value()) {
    prompt += "- rewrite_strength: " + *style.rewriteStrength + "\n";
  }
  if (style.audience.has_value()) {
    prompt += "- audience: " + *style.audience + "\n";
  }
  if (style.locale.has_value()) {
    prompt += "- locale: " + *style.locale + "\n";
  }
  if (style.terminologyRigidity.has_value()) {
    prompt += "- terminology_rigidity: " + *style.terminologyRigidity + "\n";
  }
  if (style.preserveFormatting.has_value()) {
    prompt += std::string("- preserve_formatting: ") +
              (*style.preserveFormatting ? "true" : "false") + "\n";
  }
  if (style.preserveExamples.has_value()) {
    prompt += std::string("- preserve_examples: ") +
              (*style.preserveExamples ? "true" : "false") + "\n";
  }
}

std::string BuildUserPrompt(const std::string& text, const SemanticStyle& style) {
  std::string prompt = std::string(prompts::kNormalizeTextIntro);
  prompt += std::string(prompts::kRequestedSemanticStyleLabel);
  AppendStyleDirectives(prompt, style);
  prompt += std::string(prompts::kTextLabel);
  prompt += text;
  return prompt;
}

std::string BuildBlockUserPrompt(const BlockNormalizationRequest& request) {
  std::string prompt = std::string(prompts::kNormalizeBlockIntro);
  prompt += std::string(prompts::kBlockIdLabel) + request.source_block.Id() +
            "\n";
  prompt += std::string(prompts::kBlockTypeLabel) +
            std::string(BlockTypeToString(request.source_block.type())) + "\n";
  prompt += std::string(prompts::kPreservePlaceholdersLabel);
  for (const auto& param : request.source_block.templ().ExtractParamNames()) {
    prompt += "- {{" + param + "}}\n";
  }
  prompt += std::string(prompts::kRequestedSemanticStyleLabel);
  AppendStyleDirectives(prompt, request.style);
  prompt += std::string(prompts::kTemplateLabel);
  prompt += request.source_block.templ().Content();
  return prompt;
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

}  // namespace

OpenAiCompatibleNormalizer::OpenAiCompatibleNormalizer(
    OpenAiCompatibleConfig config, std::shared_ptr<IHttpTransport> transport)
    : config_(std::move(config)), transport_(std::move(transport)) {}

Result<std::string> OpenAiCompatibleNormalizer::Normalize(
    const std::string& text, const SemanticStyle& style) const {
  if (auto err = ValidateConfig(); err.is_error()) {
    return Result<std::string>(std::move(err));
  }
  if (!transport_) {
    return Result<std::string>(
        Error{ErrorCode::StorageError, "OpenAI-compatible transport is not configured"});
  }
  if (text.empty()) {
    return Result<std::string>(
        Error{ErrorCode::InvalidParamType, "Normalization input text is empty"});
  }
  if (style.isEmpty()) {
    return Result<std::string>(
        Error{ErrorCode::InvalidParamType, "Semantic style is empty"});
  }

  auto http_request = BuildRequest(text, style);
  if (http_request.HasError()) {
    return Result<std::string>(http_request.error());
  }

  auto response = transport_->PostJson(http_request.value());
  if (response.HasError()) {
    return Result<std::string>(response.error());
  }

  return ParseResponse(response.value());
}

Result<NormalizedBlockData> OpenAiCompatibleNormalizer::NormalizeBlock(
    const BlockNormalizationRequest& request) const {
  if (auto err = ValidateConfig(); err.is_error()) {
    return Result<NormalizedBlockData>(std::move(err));
  }
  if (!transport_) {
    return Result<NormalizedBlockData>(
        Error{ErrorCode::StorageError, "OpenAI-compatible transport is not configured"});
  }
  if (request.source_block.templ().Content().empty()) {
    return Result<NormalizedBlockData>(
        Error{ErrorCode::InvalidParamType, "Block normalization input template is empty"});
  }
  if (request.style.isEmpty()) {
    return Result<NormalizedBlockData>(
        Error{ErrorCode::InvalidParamType, "Semantic style is empty"});
  }

  auto http_request = BuildBlockRequest(request);
  if (http_request.HasError()) {
    return Result<NormalizedBlockData>(http_request.error());
  }

  auto response = transport_->PostJson(http_request.value());
  if (response.HasError()) {
    return Result<NormalizedBlockData>(response.error());
  }

  auto normalized = ParseResponse(response.value());
  if (normalized.HasError()) {
    return Result<NormalizedBlockData>(normalized.error());
  }

  return Result<NormalizedBlockData>(NormalizedBlockData{
      .templ = normalized.value(),
      .description = std::nullopt,
      .language = request.style.targetLanguage,
  });
}

std::string OpenAiCompatibleNormalizer::Fingerprint() const {
  std::ostringstream stream;
  stream << "openai-compatible|"
         << config_.base_url << "|" << config_.model << "|block-normalizer-v1";
  return stream.str();
}

Error OpenAiCompatibleNormalizer::ValidateConfig() const {
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

Result<HttpRequest> OpenAiCompatibleNormalizer::BuildRequest(
    const std::string& text, const SemanticStyle& style) const {
  const OpenAiChatCompletionRequest payload{
      .model = config_.model,
      .messages =
          {
              {.role = "system", .content = BuildSystemPrompt()},
              {.role = "user", .content = BuildUserPrompt(text, style)},
          },
  };

  HttpRequest request{
      .url = JoinUrl(config_.base_url, config_.endpoint_path),
      .headers =
          {
              {"Authorization", "Bearer " + config_.api_key},
              {"Content-Type", "application/json"},
          },
      .body = rfl::json::write(payload),
  };

  if (config_.organization.has_value() && !config_.organization->empty()) {
    request.headers["OpenAI-Organization"] = *config_.organization;
  }

  return Result<HttpRequest>(std::move(request));
}

Result<HttpRequest> OpenAiCompatibleNormalizer::BuildBlockRequest(
    const BlockNormalizationRequest& request) const {
  const OpenAiChatCompletionRequest payload{
      .model = config_.model,
      .messages =
          {
              {.role = "system", .content = BuildBlockSystemPrompt()},
              {.role = "user", .content = BuildBlockUserPrompt(request)},
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

Result<std::string> OpenAiCompatibleNormalizer::ParseResponse(
    const HttpResponse& response) const {
  if (response.status_code < 200 || response.status_code >= 300) {
    return Result<std::string>(
        Error{ErrorCode::StorageError,
              "OpenAI-compatible normalization request failed with status " +
                  std::to_string(response.status_code)});
  }

  const auto parsed = rfl::json::read<OpenAiChatCompletionResponse>(response.body);
  if (!parsed) {
    return Result<std::string>(
        Error{ErrorCode::InvalidParamType,
              "Failed to parse OpenAI-compatible normalization response JSON"});
  }
  if (parsed->choices.empty()) {
    return Result<std::string>(
        Error{ErrorCode::InvalidParamType,
              "OpenAI-compatible normalization response has no choices"});
  }

  const auto& message = parsed->choices.front().message;
  if (!message.content.has_value() || message.content->empty()) {
    return Result<std::string>(Error{
        ErrorCode::InvalidParamType,
        "OpenAI-compatible normalization response does not contain message content"});
  }

  return Result<std::string>(*message.content);
}

}  // namespace tf::ai
