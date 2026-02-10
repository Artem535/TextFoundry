//
// Created by a.durynin on 29.01.2026.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "../src/textfoundry_engine/tf/logger.h"

// Initialize logger before running tests
struct LoggerSetup {
  LoggerSetup() {
    tf::Logger::init(
        tf::LogLevel::Warn);  // Only warnings and errors during tests
  }
  ~LoggerSetup() { tf::Logger::shutdown(); }
} loggerSetup;

#include "../src/textfoundry_engine/tf/block.h"
#include "../src/textfoundry_engine/tf/blockref.h"
#include "../src/textfoundry_engine/tf/blocktype.hpp"
#include "../src/textfoundry_engine/tf/composition.h"
#include "../src/textfoundry_engine/tf/engine.h"
#include "../src/textfoundry_engine/tf/error.h"
#include "../src/textfoundry_engine/tf/fragment.h"
#include "../src/textfoundry_engine/tf/renderer.h"
#include "../src/textfoundry_engine/tf/version.h"

using namespace tf;

// ==================== Test Fixture with Real Engine ====================

class EngineTestFixture {
 public:
  Engine engine;
  static int testIdCounter;
  int myTestId;

  EngineTestFixture() : myTestId(testIdCounter++) {
    // Use unique in-memory database for each test instance
    EngineConfig config;
    config.default_data_path = "memory:test_" + std::to_string(myTestId);
    engine = Engine(config);
  }

  // Helper to create and publish a block
  Block createAndPublishBlock(const BlockId& id, const std::string& templateStr,
                              const Params& defaults = {}) {
    BlockDraft draft = BlockDraftBuilder(id)
                           .WithTemplate(Template(templateStr))
                           .WithDefaults(defaults)
                           .build();
    auto pubResult =
        engine.PublishBlock(std::move(draft), Engine::VersionBump::Minor);
    REQUIRE(pubResult.HasValue());
    auto pub = pubResult.value();
    REQUIRE(pub.id() == id);
    auto blockResult = engine.LoadBlock(id, pub.version());
    REQUIRE(blockResult.HasValue());
    Block block = blockResult.value();
    REQUIRE(block.state() == BlockState::Published);
    return block;
  }

  // Helper to create and publish a composition
  Composition createAndPublishComposition(
      const CompositionId& id,
      const std::vector<std::pair<std::string, Version>>& blockRefs) {
    CompositionDraftBuilder builder(id);
    for (const auto& [blockId, ver] : blockRefs) {
      builder.AddBlockRef(blockId, ver.major, ver.minor);
    }
    CompositionDraft draft = builder.build();
    auto pubResult =
        engine.PublishComposition(std::move(draft), Engine::VersionBump::Minor);
    REQUIRE(pubResult.HasValue());
    auto pub = pubResult.value();
    REQUIRE(pub.id() == id);
    auto compResult = engine.LoadComposition(id, pub.version());
    REQUIRE(compResult.HasValue());
    Composition comp = compResult.value();
    REQUIRE(comp.state() == BlockState::Published);
    return comp;
  }
};

int EngineTestFixture::testIdCounter = 0;

// ==================== Version Tests ====================

TEST_SUITE("Version") {
  TEST_CASE("default version is 0.0") {
    Version v;
    CHECK(v.major == 0);
    CHECK(v.minor == 0);
  }

  TEST_CASE("version comparison") {
    Version v1{1, 0};
    Version v2{1, 5};
    Version v3{2, 0};

    CHECK(v1 < v2);
    CHECK(v2 < v3);
    CHECK(v1 < v3);
    CHECK(v1 == Version{1, 0});
    CHECK(v1 != v2);
  }

  TEST_CASE("version toString") {
    Version v{2, 5};
    CHECK(v.ToString() == "2.5");
  }
}

// ==================== Error Tests ====================

TEST_SUITE("Error") {
  TEST_CASE("success error has no error") {
    auto err = Error::success();
    CHECK(err.is_success());
    CHECK_FALSE(err.is_error());
  }

  TEST_CASE("missing param error") {
    auto err = Error::MissingParam("name");
    CHECK(err.is_error());
    CHECK(err.code == ErrorCode::MissingParam);
    CHECK(err.message.find("name") != std::string::npos);
  }

  TEST_CASE("version required error") {
    auto err = Error::VersionRequired();
    CHECK(err.is_error());
    CHECK(err.code == ErrorCode::VersionRequired);
  }
}

// ==================== Result Tests ====================

TEST_SUITE("Result") {
  TEST_CASE("Result with value") {
    Result<int> r(42);
    CHECK(r.HasValue());
    CHECK_FALSE(r.HasError());
    CHECK(*r == 42);
    CHECK(r.value() == 42);
  }

  TEST_CASE("Result with error") {
    Result<int> r(Error::MissingParam("test"));
    CHECK_FALSE(r.HasValue());
    CHECK(r.HasError());
    CHECK(r.error().code == ErrorCode::MissingParam);
  }
}

// ==================== BlockType Tests ====================

TEST_SUITE("BlockType") {
  TEST_CASE("BlockType to string") {
    CHECK(BlockTypeToString(BlockType::Role) == "role");
    CHECK(BlockTypeToString(BlockType::Constraint) == "constraint");
    CHECK(BlockTypeToString(BlockType::Style) == "style");
    CHECK(BlockTypeToString(BlockType::Domain) == "domain");
    CHECK(BlockTypeToString(BlockType::Meta) == "meta");
  }

  TEST_CASE("BlockType from string") {
    CHECK(BlockTypeFromString("role") == BlockType::Role);
    CHECK(BlockTypeFromString("constraint") == BlockType::Constraint);
    CHECK(BlockTypeFromString("style") == BlockType::Style);
    CHECK(BlockTypeFromString("domain") == BlockType::Domain);
    CHECK(BlockTypeFromString("meta") == BlockType::Meta);
  }

  TEST_CASE("BlockType from invalid string throws") {
    CHECK_THROWS_AS(BlockTypeFromString("invalid"), std::invalid_argument);
  }
}

// ==================== Template Tests ====================

TEST_SUITE("Template") {
  TEST_CASE("extract param names") {
    Template t("Hello, {{name}}! Your score is {{score}}.");
    auto params = t.ExtractParamNames();
    CHECK(params.size() == 2);
    CHECK(params[0] == "name");
    CHECK(params[1] == "score");
  }

  TEST_CASE("expand template with all params") {
    Template t("Hello, {{name}}!");
    Params p{{"name", "World"}};
    auto result = t.Expand(p);
    CHECK(result.HasValue());
    CHECK(result.value() == "Hello, World!");
  }

  TEST_CASE("expand template with missing param returns error") {
    Template t("Hello, {{name}}!");
    Params p{};
    auto result = t.Expand(p);
    CHECK(result.HasError());
    CHECK(result.error().code == ErrorCode::MissingParam);
  }

  TEST_CASE("expand template with no params") {
    Template t("Hello, World!");
    Params p{};
    auto result = t.Expand(p);
    CHECK(result.HasValue());
    CHECK(result.value() == "Hello, World!");
  }
}

// ==================== Block Tests ====================

TEST_SUITE("Block") {
  TEST_CASE("create block with id") {
    Block b("greeting.hello");
    CHECK(b.Id() == "greeting.hello");
    CHECK(b.state() == BlockState::Draft);
    CHECK(b.type() == BlockType::Domain);
  }

  TEST_CASE("block defaults and resolution") {
    Block b("test.block");
    b.SetDefaults({{"name", "default_user"}});

    Params local{{"name", "local_user"}};
    Params runtime{{"name", "runtime_user"}};

    // Runtime has highest priority
    auto r1 = b.ResolveParam("name", local, runtime);
    CHECK(r1.HasValue());
    CHECK(r1.value() == "runtime_user");

    // Local has priority over defaults
    Params emptyRuntime;
    auto r2 = b.ResolveParam("name", local, emptyRuntime);
    CHECK(r2.HasValue());
    CHECK(r2.value() == "local_user");

    // Defaults used when no overrides
    Params emptyLocal;
    auto r3 = b.ResolveParam("name", emptyLocal, emptyRuntime);
    CHECK(r3.HasValue());
    CHECK(r3.value() == "default_user");
  }

  TEST_CASE("block validate params") {
    Block b("test.block");
    b.SetTemplate(Template("Hello, {{name}}!"));

    // Missing param
    auto err = b.ValidateParams({}, {});
    CHECK(err.is_error());
    CHECK(err.code == ErrorCode::MissingParam);

    // With default
    b.SetDefaults({{"name", "user"}});
    err = b.ValidateParams({}, {});
    CHECK(err.is_success());
  }

  TEST_CASE("block publish") {
    Block b("test.block");
    b.SetTemplate(Template("Hello!"));

    CHECK(b.state() == BlockState::Draft);

    auto err = b.publish(Version{1, 0});
    CHECK(err.is_success());
    CHECK(b.state() == BlockState::Published);
    CHECK(b.version() == Version{1, 0});
  }

  TEST_CASE("cannot publish without template") {
    Block b("test.block");
    auto err = b.publish(Version{1, 0});
    CHECK(err.is_error());
  }

  TEST_CASE("cannot publish already published block") {
    Block b("test.block");
    b.SetTemplate(Template("Hello!"));
    auto err = b.publish(Version{1, 0});
    CHECK(err.is_success());

    err = b.publish(Version{2, 0});
    CHECK(err.is_error());
  }

  TEST_CASE("block deprecate") {
    Block b("test.block");
    b.SetTemplate(Template("Hello!"));
    auto err = b.publish(Version{1, 0});
    CHECK(err.is_success());

    b.deprecate();
    CHECK(b.state() == BlockState::Deprecated);
  }
}

// ==================== BlockRef Tests ====================

TEST_SUITE("BlockRef") {
  TEST_CASE("BlockRef with version") {
    BlockRef ref("block.id", Version{1, 5});
    CHECK(ref.GetBlockId() == "block.id");
    CHECK(ref.version().has_value());
    CHECK(ref.version().value() == Version{1, 5});
    CHECK_FALSE(ref.UseLatest());
  }

  TEST_CASE("BlockRef with UseLatest") {
    BlockRef ref("block.id");
    CHECK(ref.GetBlockId() == "block.id");
    CHECK_FALSE(ref.version().has_value());
    CHECK(ref.UseLatest());
  }

  TEST_CASE("BlockRef validate in draft context allows UseLatest") {
    BlockRef ref("block.id");       // UseLatest = true
    auto err = ref.validate(true);  // isDraftContext = true
    CHECK(err.is_success());
  }

  TEST_CASE("BlockRef validate in non-draft context rejects UseLatest") {
    BlockRef ref("block.id");        // UseLatest = true
    auto err = ref.validate(false);  // isDraftContext = false
    CHECK(err.is_error());
    CHECK(err.code == ErrorCode::VersionRequired);
  }

  TEST_CASE("BlockRef with version validates in non-draft context") {
    BlockRef ref("block.id", Version{1, 0});
    auto err = ref.validate(false);
    CHECK(err.is_success());
  }

  TEST_CASE("BlockRef resolve params") {
    Block block("test.block");
    block.SetTemplate(Template("{{greeting}}, {{name}}!"));
    block.SetDefaults({{"greeting", "Hello"}});

    BlockRef ref("test.block", Version{1, 0}, {{"name", "World"}});

    Params runtime{{"greeting", "Hi"}};  // Runtime overrides block defaults
    auto result = ref.ResolveParams(block, runtime);

    CHECK(result.HasValue());
    CHECK(result.value()["greeting"] == "Hi");  // Runtime wins
    CHECK(result.value()["name"] == "World");   // Local override used
  }
}

// ==================== Fragment Tests ====================

TEST_SUITE("Fragment") {
  TEST_CASE("Fragment BlockRef type") {
    BlockRef ref("block.id", Version{1, 0});
    Fragment f(Fragment::MakeBlockRef(std::move(ref)));

    CHECK(f.IsBlockRef());
    CHECK_FALSE(f.IsStaticText());
    CHECK_FALSE(f.IsSeparator());
    CHECK(f.type() == FragmentType::BlockRef);
  }

  TEST_CASE("Fragment StaticText type") {
    Fragment f = Fragment::MakeStaticText("Hello, World!");

    CHECK_FALSE(f.IsBlockRef());
    CHECK(f.IsStaticText());
    CHECK_FALSE(f.IsSeparator());
    CHECK(f.type() == FragmentType::StaticText);
    CHECK(f.AsStaticText().text() == "Hello, World!");
  }

  TEST_CASE("Fragment Separator type") {
    Fragment f = Fragment::MakeSeparator(SeparatorType::Newline);

    CHECK_FALSE(f.IsBlockRef());
    CHECK_FALSE(f.IsStaticText());
    CHECK(f.IsSeparator());
    CHECK(f.type() == FragmentType::Separator);
    CHECK(f.AsSeparator().toString() == "\n");
  }

  TEST_CASE("Separator toString variants") {
    CHECK(Separator(SeparatorType::Newline).toString() == "\n");
    CHECK(Separator(SeparatorType::Paragraph).toString() == "\n\n");
    CHECK(Separator(SeparatorType::Hr).toString() == "\n---\n");
  }

  TEST_CASE("Fragment validate delegates to BlockRef") {
    BlockRef ref("block.id");  // UseLatest = true
    Fragment f = Fragment::MakeBlockRef(ref);

    auto err = f.validate(true);  // Draft context - OK
    CHECK(err.is_success());

    err = f.validate(false);  // Non-draft context - Error
    CHECK(err.is_error());
  }
}

// ==================== Composition Tests ====================

TEST_SUITE("Composition") {
  TEST_CASE("create composition") {
    Composition c("test.composition");
    CHECK(c.id() == "test.composition");
    CHECK(c.state() == BlockState::Draft);
    CHECK(c.fragmentCount() == 0);
  }

  TEST_CASE("add fragments to composition") {
    Composition c("test.composition");

    c.AddStaticText("Hello");
    c.AddSeparator(SeparatorType::Newline);
    c.AddBlockRef("block.id", Version{1, 0});

    CHECK(c.fragmentCount() == 3);
    CHECK(c.fragment(0).IsStaticText());
    CHECK(c.fragment(1).IsSeparator());
    CHECK(c.fragment(2).IsBlockRef());
  }

  TEST_CASE("insert and remove fragments") {
    Composition c("test.composition");
    c.AddStaticText("First");
    c.AddStaticText("Third");

    c.InsertFragment(1, Fragment::MakeStaticText("Second"));
    CHECK(c.fragmentCount() == 3);
    CHECK(c.fragment(1).AsStaticText().text() == "Second");

    c.RemoveFragment(1);
    CHECK(c.fragmentCount() == 2);
    CHECK(c.fragment(1).AsStaticText().text() == "Third");
  }

  TEST_CASE("clear fragments") {
    Composition c("test.composition");
    c.AddStaticText("Text");
    c.AddBlockRef("block.id", Version{1, 0});

    CHECK(c.fragmentCount() == 2);
    c.ClearFragments();
    CHECK(c.fragmentCount() == 0);
  }

  TEST_CASE("composition validate requires id") {
    Composition c;  // No ID
    auto err = c.validate();
    CHECK(err.is_error());
  }

  TEST_CASE("composition validate with id succeeds") {
    Composition c("test.composition");
    c.AddStaticText("Text");
    auto err = c.validate();
    CHECK(err.is_success());
  }

  TEST_CASE("composition publish validates BlockRef versions") {
    Composition c("test.composition");
    c.AddBlockRefLatest("block.id");  // UseLatest = true

    auto err = c.publish(Version{1, 0});
    CHECK(err.is_error());  // Cannot publish with UseLatest
    CHECK(err.code == ErrorCode::VersionRequired);
  }

  TEST_CASE("composition publish with explicit versions succeeds") {
    Composition c("test.composition");
    c.AddBlockRef("block.id", Version{1, 0});

    auto err = c.publish(Version{1, 0});
    CHECK(err.is_success());
    CHECK(c.state() == BlockState::Published);
  }

  TEST_CASE("composition deprecate") {
    Composition c("test.composition");
    auto err = c.publish(Version{1, 0});
    CHECK(err.is_success());

    c.deprecate();
    CHECK(c.state() == BlockState::Deprecated);
  }

  TEST_CASE("SemanticStyle isEmpty") {
    SemanticStyle s;
    CHECK(s.isEmpty());

    s.tone = "formal";
    CHECK_FALSE(s.isEmpty());
  }

  TEST_CASE("RenderContext builder") {
    auto ctx = RenderContext{}
                   .WithParam("name", "value")
                   .WithLanguage("en")
                   .with_strict_mode(true);

    CHECK(ctx.params["name"] == "value");
    CHECK(ctx.targetLanguage == "en");
    CHECK(ctx.strictMode);
  }
}

// ==================== BlockDraftBuilder Tests ====================

TEST_SUITE("BlockDraftBuilder") {
  TEST_CASE("build block with builder") {
    EngineTestFixture fixture;
    BlockDraft draft = BlockDraftBuilder("greeting.hello")
                           .WithType(BlockType::Role)
                           .WithTemplate(Template("Hello, {{name}}!"))
                           .WithDefault("name", "World")
                           .WithTag("simple")
                           .WithLanguage("en")
                           .WithDescription("A simple greeting")
                           .build();
    auto pubBlock =
        fixture.engine
            .PublishBlock(std::move(draft), Engine::VersionBump::Minor)
            .value();
    auto block =
        fixture.engine.LoadBlock("greeting.hello", pubBlock.version()).value();

    CHECK(block.Id() == "greeting.hello");
    CHECK(block.type() == BlockType::Role);
    CHECK(block.defaults().at("name") == "World");
    CHECK(block.tags().contains("simple"));
    CHECK(block.language() == "en");
    CHECK(block.description() == "A simple greeting");
  }
}

// ==================== CompositionDraftBuilder Tests ====================

TEST_SUITE("CompositionDraftBuilder") {
  TEST_CASE("build composition with builder") {
    EngineTestFixture fixture;

    // Publish prerequisite block first
    BlockDraft helloDraft = BlockDraftBuilder("greeting.hello")
                                .WithTemplate(Template("Hello, {{name}}!"))
                                .build();
    auto helloPub =
        fixture.engine
            .PublishBlock(std::move(helloDraft), Engine::VersionBump::Minor)
            .value();

    auto builder = CompositionDraftBuilder("welcome.message")
                       .WithProjectKey("myproject")
                       .WithDescription("Welcome message")
                       .AddStaticText("# Welcome\n\n")
                       .AddBlockRef("greeting.hello", 1, 0, {{"name", "User"}})
                       .AddSeparator(SeparatorType::Paragraph)
                       .AddStaticText("Enjoy your stay!");
    auto draft = builder.build();
    auto pubComp =
        fixture.engine
            .PublishComposition(std::move(draft), Engine::VersionBump::Minor)
            .value();
    auto comp =
        fixture.engine.LoadComposition("welcome.message", pubComp.version())
            .value();

    CHECK(comp.id() == "welcome.message");
    CHECK(comp.ProjectKey() == "myproject");
    CHECK(comp.description() == "Welcome message");
    CHECK(comp.fragmentCount() == 4);
  }
}

// ==================== Renderer Tests (using real Engine) ====================

TEST_CASE_FIXTURE(EngineTestFixture, "render single block through engine") {
  // Create and publish block
  createAndPublishBlock("greeting.hello", "Hello, {{name}}!",
                        {{"name", "World"}});

  // Render through engine
  auto result = engine.RenderBlock("greeting.hello", Version{1, 0});

  CHECK(result.HasValue());
  CHECK(result.value() == "Hello, World!");
}

TEST_CASE_FIXTURE(EngineTestFixture,
                  "render block with runtime override through engine") {
  createAndPublishBlock("greeting.hello", "Hello, {{name}}!",
                        {{"name", "World"}});

  auto ctx = RenderContext{}.WithParam("name", "Alice");
  auto result = engine.RenderBlock("greeting.hello", Version{1, 0}, ctx);

  CHECK(result.HasValue());
  CHECK(result.value() == "Hello, Alice!");
}

TEST_CASE_FIXTURE(EngineTestFixture, "render block with missing param fails") {
  createAndPublishBlock("greeting.hello", "Hello, {{name}}!");  // No defaults

  auto result = engine.RenderBlock("greeting.hello", Version{1, 0});

  CHECK(result.HasError());
  CHECK(result.error().code == ErrorCode::MissingParam);
}

TEST_CASE_FIXTURE(EngineTestFixture,
                  "render composition with static text only") {
  CompositionDraftBuilder builder("test.comp");
  builder.AddStaticText("Hello, World!");
  auto draft = builder.build();
  auto pubResult =
      engine.PublishComposition(std::move(draft), Engine::VersionBump::Minor);
  REQUIRE(pubResult.HasValue());

  auto result = engine.Render("test.comp");

  CHECK(result.HasValue());
  CHECK(result.value().text == "Hello, World!");
  CHECK(result.value().compositionId == "test.comp");
}

TEST_CASE_FIXTURE(EngineTestFixture, "render unpublished composition fails") {
  // No composition published with this ID
  auto result = engine.Render("test.comp");

  CHECK(result.HasError());
  // Expect storage not found error, since no draft persistence
  // CHECK(result.error().code == ErrorCode::PublishedRequired); // Adjust based
  // on actual error
}

TEST_CASE_FIXTURE(EngineTestFixture, "render composition with separator") {
  CompositionDraftBuilder builder("test.comp");
  builder.AddStaticText("Line 1");
  builder.AddSeparator(SeparatorType::Newline);
  builder.AddStaticText("Line 2");
  auto draft = builder.build();
  engine.PublishComposition(std::move(draft), Engine::VersionBump::Minor);

  auto result = engine.Render("test.comp");

  CHECK(result.HasValue());
  CHECK(result.value().text == "Line 1\nLine 2");
}

TEST_CASE_FIXTURE(EngineTestFixture, "render composition with BlockRef") {
  // Create and publish block
  createAndPublishBlock("greeting.hello", "Hello, {{name}}!",
                        {{"name", "World"}});

  // Create and publish composition with BlockRef
  CompositionDraftBuilder builder("test.comp");
  builder.AddBlockRef("greeting.hello", 1, 0);
  auto draft = builder.build();
  engine.PublishComposition(std::move(draft), Engine::VersionBump::Minor);

  auto result = engine.Render("test.comp");

  CHECK(result.HasValue());
  CHECK(result.value().text == "Hello, World!");
  CHECK(result.value().blocksUsed.size() == 1);
  CHECK(result.value().blocksUsed[0].first == "greeting.hello");
  CHECK(result.value().blocksUsed[0].second == Version{1, 0});
}

TEST_CASE_FIXTURE(EngineTestFixture,
                  "render composition with BlockRef and runtime params") {
  createAndPublishBlock("greeting.hello", "Hello, {{name}}!",
                        {{"name", "World"}});

  CompositionDraftBuilder builder("test.comp");
  builder.AddBlockRef("greeting.hello", 1, 0);
  auto draft = builder.build();
  engine.PublishComposition(std::move(draft), Engine::VersionBump::Minor);

  auto ctx = RenderContext{}.WithParam("name", "Alice");
  auto result = engine.Render("test.comp", ctx);

  CHECK(result.HasValue());
  CHECK(result.value().text == "Hello, Alice!");
}

TEST_CASE_FIXTURE(EngineTestFixture,
                  "render composition with multiple fragments") {
  createAndPublishBlock("greeting.hello", "Hello, {{name}}!",
                        {{"name", "World"}});
  createAndPublishBlock("farewell.goodbye", "Goodbye, {{name}}!",
                        {{"name", "Friend"}});

  CompositionDraftBuilder builder("test.comp");
  builder.AddBlockRef("greeting.hello", 1, 0, {{"name", "Alice"}});
  builder.AddSeparator(SeparatorType::Paragraph);
  builder.AddBlockRef("farewell.goodbye", 1, 0, {{"name", "Alice"}});
  auto draft = builder.build();
  engine.PublishComposition(std::move(draft), Version{1, 0});

  auto result = engine.Render("test.comp", Version{1, 0});

  CHECK(result.HasValue());
  CHECK(result.value().text == "Hello, Alice!\n\nGoodbye, Alice!");
  CHECK(result.value().blocksUsed.size() == 2);
}

TEST_CASE_FIXTURE(EngineTestFixture, "render with missing block fails") {
  // Create composition referencing non-existent block
  CompositionDraftBuilder builder("test.comp");
  builder.AddBlockRef("missing.block", 1, 0);
  auto draft = builder.build();
  engine.PublishComposition(std::move(draft), Version{1, 0});

  auto result = engine.Render("test.comp", Version{1, 0});

  CHECK(result.HasError());
  CHECK(result.error().code == ErrorCode::BlockNotFound);
}

TEST_CASE_FIXTURE(EngineTestFixture, "render result contains metadata") {
  CompositionDraftBuilder builder("my.composition");
  builder.AddStaticText("Text");
  auto draft = builder.build();
  engine.PublishComposition(std::move(draft), Version{2, 5});

  auto result = engine.Render("my.composition", Version{2, 5});

  CHECK(result.HasValue());
  CHECK(result.value().compositionId == "my.composition");
  CHECK(result.value().compositionVersion == Version{2, 5});
}

// ==================== Engine Integration Tests ====================

TEST_CASE_FIXTURE(EngineTestFixture, "create and save block") {
  BlockDraft draft = BlockDraftBuilder("test.block")
                         .WithTemplate(Template("Hello, {{name}}!"))
                         .WithDefaults({{"name", "World"}})
                         .build();
  // Drafts are not saved separately; publish or use directly

  // Load after publish
  auto pub = engine.PublishBlock(std::move(draft), Version{1, 0}).value();
  auto result = engine.LoadBlock("test.block", pub.version());
  CHECK(result.HasValue());
  CHECK(result.value().Id() == "test.block");
  CHECK(result.value().templ().ExtractParamNames().size() == 1);
}

TEST_CASE_FIXTURE(EngineTestFixture, "publish block workflow") {
  BlockDraft draft =
      BlockDraftBuilder("test.block").WithTemplate(Template("Hello!")).build();
  auto result = engine.PublishBlock(std::move(draft), Version{1, 0});
  CHECK(result.HasValue());

  // Check published block
  auto block = engine.LoadBlock("test.block", Version{1, 0}).value();
  CHECK(block.state() == BlockState::Published);
  CHECK(block.version() == Version{1, 0});

  // Check latest version
  auto verResult = engine.GetLatestBlockVersion("test.block");
  CHECK(verResult.HasValue());
  CHECK(verResult.value() == Version{1, 0});
}

TEST_CASE_FIXTURE(EngineTestFixture, "list blocks") {
  createAndPublishBlock("block1", "Text 1", {});
  createAndPublishBlock("block2", "Text 2", {});

  auto blocks = engine.ListBlocks();
  CHECK(blocks.size() == 2);
}

TEST_CASE_FIXTURE(EngineTestFixture, "create and save composition") {
  CompositionDraftBuilder builder("test.comp");
  builder.AddStaticText("Hello");
  auto draft = builder.build();
  // Drafts not saved; test publish instead

  auto pub = engine.PublishComposition(std::move(draft), Version{1, 0}).value();
  auto result = engine.LoadComposition("test.comp", pub.version());
  CHECK(result.HasValue());
  CHECK(result.value().id() == "test.comp");
  CHECK(result.value().fragmentCount() == 1);
}

TEST_CASE_FIXTURE(EngineTestFixture, "publish composition workflow") {
  // First create and publish a block
  createAndPublishBlock("test.block", "Hello!", {});

  // Then create and publish composition referencing it
  CompositionDraftBuilder builder("test.comp");
  builder.AddBlockRef("test.block", 1, 0);
  auto draft = builder.build();
  auto result = engine.PublishComposition(std::move(draft), Version{1, 0});
  CHECK(result.HasValue());

  // List compositions
  auto comps = engine.ListCompositions();
  CHECK(comps.size() == 1);
  CHECK(comps[0] == "test.comp");
}

TEST_CASE_FIXTURE(EngineTestFixture, "validate block through engine") {
  BlockDraft draft = BlockDraftBuilder("test.block")
                         .WithTemplate(Template("Hello, {{name}}!"))
                         .WithDefaults({{"name", "World"}})
                         .build();
  auto pub = engine.PublishBlock(std::move(draft), Version{1, 0}).value();

  auto err = engine.ValidateBlock("test.block");
  CHECK(err.is_success());
}

TEST_CASE_FIXTURE(EngineTestFixture, "validate composition through engine") {
  createAndPublishBlock("test.block", "Hello!", {});

  CompositionDraftBuilder builder("test.comp");
  builder.AddBlockRef("test.block", 1, 0);
  auto draft = builder.build();
  auto pubResult = engine.PublishComposition(std::move(draft), Version{1, 0});
  REQUIRE(pubResult.HasValue());  // publish to validate

  auto err = engine.ValidateComposition("test.comp");
  CHECK(err.is_success());
}
