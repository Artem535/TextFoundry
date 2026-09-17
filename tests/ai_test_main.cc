//
// AI-integration tests: exercise textfoundry_ai adapters against a fake
// HTTP transport. Split out of test_main.cc so the engine-only tests can
// move to the TextFoundryEngine repository without an textfoundry_ai
// dependency riding along.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "../src/textfoundry_engine/tf/block_generation.h"
#include "../src/textfoundry_engine/tf/block_type.hpp"
#include "../src/textfoundry_engine/tf/error.h"
#include "../src/textfoundry_ai/openai_compatible_block_generator.h"

using namespace tf;

namespace {

class FakeHttpTransport final : public tf::ai::IHttpTransport {
 public:
  explicit FakeHttpTransport(Result<tf::ai::HttpResponse> response)
      : response_(std::move(response)) {}

  [[nodiscard]] Result<tf::ai::HttpResponse> PostJson(
      const tf::ai::HttpRequest& request) const override {
    last_request = request;
    if (response_.HasError()) {
      return Result<tf::ai::HttpResponse>(response_.error());
    }
    return Result<tf::ai::HttpResponse>(response_.value());
  }

  mutable std::optional<tf::ai::HttpRequest> last_request;

 private:
  Result<tf::ai::HttpResponse> response_;
};

}  // namespace

TEST_SUITE("OpenAiCompatibleBlockGenerator") {
  TEST_CASE("builds chat completions request and parses structured response") {
    auto transport = std::make_shared<FakeHttpTransport>(
        Result<tf::ai::HttpResponse>(tf::ai::HttpResponse{
            .status_code = 200,
            .body =
                R"({"choices":[{"message":{"role":"assistant","content":"{\"id\":\"role.code_reviewer\",\"type\":\"role\",\"language\":\"en\",\"description\":\"Reviews code changes\",\"templ\":\"Review {{subject}} for {{focus}}.\",\"defaults\":{\"focus\":\"correctness\"},\"tags\":[\"review\",\"quality\"]}"}}]})",
        }));

    tf::ai::OpenAiCompatibleBlockGenerator generator(
        {.base_url = "https://example.test/v1",
         .model = "gpt-4.1-mini",
         .api_key = "secret"},
        transport);

    auto result = generator.GenerateBlock(
        {.prompt = "Create a code reviewer block",
         .preferred_type = BlockType::Role,
         .existing_block_ids = {"role.existing"}});
    REQUIRE(result.HasValue());
    REQUIRE(transport->last_request.has_value());

    CHECK(transport->last_request->url ==
          "https://example.test/v1/chat/completions");
    CHECK(transport->last_request->headers.at("Authorization") ==
          "Bearer secret");
    CHECK(transport->last_request->body.find("\"response_format\"") !=
          std::string::npos);
    CHECK(transport->last_request->body.find("\"json_schema\"") !=
          std::string::npos);
    CHECK(result.value().id == "role.code_reviewer");
    CHECK(result.value().type == BlockType::Role);
    CHECK(result.value().templ == "Review {{subject}} for {{focus}}.");
    CHECK(result.value().defaults.at("focus") == "correctness");
    CHECK(result.value().tags.size() == 2);
  }

  TEST_CASE("returns error on non-success HTTP status") {
    auto transport = std::make_shared<FakeHttpTransport>(
        Result<tf::ai::HttpResponse>(
            tf::ai::HttpResponse{.status_code = 401, .body = "{}"}));

    tf::ai::OpenAiCompatibleBlockGenerator generator(
        {.base_url = "https://example.test/v1",
         .model = "gpt-4.1-mini",
         .api_key = "secret"},
        transport);

    auto result = generator.GenerateBlock({.prompt = "Create a block"});
    REQUIRE(result.HasError());
    CHECK(result.error().code == ErrorCode::StorageError);
  }

  TEST_CASE("builds batch request and parses structured block array response") {
    auto transport = std::make_shared<FakeHttpTransport>(
        Result<tf::ai::HttpResponse>(tf::ai::HttpResponse{
            .status_code = 200,
            .body =
                R"({"choices":[{"message":{"role":"assistant","content":"{\"blocks\":[{\"id\":\"team.role.system\",\"type\":\"role\",\"language\":\"en\",\"description\":\"System role\",\"templ\":\"You are {{assistant_name}}.\",\"defaults\":{},\"tags\":[\"system\"]},{\"id\":\"team.constraint.style\",\"type\":\"constraint\",\"language\":\"en\",\"description\":\"Style constraints\",\"templ\":\"Be concise.\",\"defaults\":{},\"tags\":[\"style\"]}]}"}}]})",
        }));

    tf::ai::OpenAiCompatibleBlockGenerator generator(
        {.base_url = "https://example.test/v1",
         .model = "gpt-4.1-mini",
         .api_key = "secret"},
        transport);

    auto result = generator.GenerateBlocks(
        {.source_text = "Long prompt",
         .preferred_language = "en",
         .namespace_prefix = "team"});
    REQUIRE(result.HasValue());
    REQUIRE(transport->last_request.has_value());

    CHECK(transport->last_request->body.find("textfoundry_block_batch") !=
          std::string::npos);
    CHECK(result.value().blocks.size() == 2);
    CHECK(result.value().blocks.at(0).id == "team.role.system");
    CHECK(result.value().blocks.at(1).type == BlockType::Constraint);
  }

  TEST_CASE("returns config validation error before transport call") {
    auto transport = std::make_shared<FakeHttpTransport>(
        Result<tf::ai::HttpResponse>(
            tf::ai::HttpResponse{.status_code = 200, .body = "{}"}));

    tf::ai::OpenAiCompatibleBlockGenerator generator(
        {.base_url = "", .model = "gpt-4.1-mini", .api_key = "secret"},
        transport);

    auto result = generator.GenerateBlock({.prompt = "Create a block"});
    REQUIRE(result.HasError());
    CHECK(result.error().code == ErrorCode::InvalidParamType);
    CHECK_FALSE(transport->last_request.has_value());
  }
}
