//
// Created by a.durynin on 29.01.2026.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "../src/textfoundry_engine/logger.h"

// Initialize logger before running tests
struct LoggerSetup {
    LoggerSetup() {
        tf::Logger::init(tf::LogLevel::Warn);  // Only warnings and errors during tests
    }
    ~LoggerSetup() {
        tf::Logger::shutdown();
    }
} loggerSetup;

#include "../src/textfoundry_engine/version.h"
#include "../src/textfoundry_engine/error.h"
#include "../src/textfoundry_engine/blocktype.hpp"
#include "../src/textfoundry_engine/block.h"
#include "../src/textfoundry_engine/blockref.h"
#include "../src/textfoundry_engine/fragment.h"
#include "../src/textfoundry_engine/composition.h"
#include "../src/textfoundry_engine/renderer.h"
#include "../src/textfoundry_engine/engine.h"

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
                                 const Params& defaults = {}, Version version = Version{1, 0}) {
        auto block = engine.createBlockDraft(id);
        block.setTemplate(Template(templateStr));
        if (!defaults.empty()) {
            block.setDefaults(defaults);
        }
        auto saveErr = engine.saveBlock(block);
        REQUIRE(saveErr.isSuccess());
        auto result = engine.publishBlock(id, version);
        REQUIRE(result.hasValue());
        return result.value();
    }

    // Helper to create and publish a composition
    Composition createAndPublishComposition(const CompositionId& id,
                                             const std::vector<std::pair<std::string, Version>>& blockRefs,
                                             Version version = Version{1, 0}) {
        auto comp = engine.createCompositionDraft(id);
        for (const auto& [blockId, ver] : blockRefs) {
            comp.addBlockRef(blockId, ver);
        }
        auto saveErr = engine.saveComposition(comp);
        REQUIRE(saveErr.isSuccess());
        auto result = engine.publishComposition(id, version);
        REQUIRE(result.hasValue());
        return result.value();
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
        CHECK(v.toString() == "2.5");
    }
}

// ==================== Error Tests ====================

TEST_SUITE("Error") {
    TEST_CASE("success error has no error") {
        auto err = Error::success();
        CHECK(err.isSuccess());
        CHECK_FALSE(err.isError());
    }

    TEST_CASE("missing param error") {
        auto err = Error::missingParam("name");
        CHECK(err.isError());
        CHECK(err.code == ErrorCode::MissingParam);
        CHECK(err.message.find("name") != std::string::npos);
    }

    TEST_CASE("version required error") {
        auto err = Error::versionRequired();
        CHECK(err.isError());
        CHECK(err.code == ErrorCode::VersionRequired);
    }
}

// ==================== Result Tests ====================

TEST_SUITE("Result") {
    TEST_CASE("Result with value") {
        Result<int> r(42);
        CHECK(r.hasValue());
        CHECK_FALSE(r.hasError());
        CHECK(*r == 42);
        CHECK(r.value() == 42);
    }

    TEST_CASE("Result with error") {
        Result<int> r(Error::missingParam("test"));
        CHECK_FALSE(r.hasValue());
        CHECK(r.hasError());
        CHECK(r.error().code == ErrorCode::MissingParam);
    }
}

// ==================== BlockType Tests ====================

TEST_SUITE("BlockType") {
    TEST_CASE("BlockType to string") {
        CHECK(blockTypeToString(BlockType::Role) == "role");
        CHECK(blockTypeToString(BlockType::Constraint) == "constraint");
        CHECK(blockTypeToString(BlockType::Style) == "style");
        CHECK(blockTypeToString(BlockType::Domain) == "domain");
        CHECK(blockTypeToString(BlockType::Meta) == "meta");
    }

    TEST_CASE("BlockType from string") {
        CHECK(blockTypeFromString("role") == BlockType::Role);
        CHECK(blockTypeFromString("constraint") == BlockType::Constraint);
        CHECK(blockTypeFromString("style") == BlockType::Style);
        CHECK(blockTypeFromString("domain") == BlockType::Domain);
        CHECK(blockTypeFromString("meta") == BlockType::Meta);
    }

    TEST_CASE("BlockType from invalid string throws") {
        CHECK_THROWS_AS(blockTypeFromString("invalid"), std::invalid_argument);
    }
}

// ==================== Template Tests ====================

TEST_SUITE("Template") {
    TEST_CASE("extract param names") {
        Template t("Hello, {{name}}! Your score is {{score}}.");
        auto params = t.extractParamNames();
        CHECK(params.size() == 2);
        CHECK(params[0] == "name");
        CHECK(params[1] == "score");
    }

    TEST_CASE("expand template with all params") {
        Template t("Hello, {{name}}!");
        Params p{{"name", "World"}};
        auto result = t.expand(p);
        CHECK(result.hasValue());
        CHECK(result.value() == "Hello, World!");
    }

    TEST_CASE("expand template with missing param returns error") {
        Template t("Hello, {{name}}!");
        Params p{};
        auto result = t.expand(p);
        CHECK(result.hasError());
        CHECK(result.error().code == ErrorCode::MissingParam);
    }

    TEST_CASE("expand template with no params") {
        Template t("Hello, World!");
        Params p{};
        auto result = t.expand(p);
        CHECK(result.hasValue());
        CHECK(result.value() == "Hello, World!");
    }
}

// ==================== Block Tests ====================

TEST_SUITE("Block") {
    TEST_CASE("create block with id") {
        Block b("greeting.hello");
        CHECK(b.id() == "greeting.hello");
        CHECK(b.state() == BlockState::Draft);
        CHECK(b.type() == BlockType::Domain);
    }

    TEST_CASE("block defaults and resolution") {
        Block b("test.block");
        b.setDefaults({{"name", "default_user"}});

        Params local{{"name", "local_user"}};
        Params runtime{{"name", "runtime_user"}};

        // Runtime has highest priority
        auto r1 = b.resolveParam("name", local, runtime);
        CHECK(r1.hasValue());
        CHECK(r1.value() == "runtime_user");

        // Local has priority over defaults
        Params emptyRuntime;
        auto r2 = b.resolveParam("name", local, emptyRuntime);
        CHECK(r2.hasValue());
        CHECK(r2.value() == "local_user");

        // Defaults used when no overrides
        Params emptyLocal;
        auto r3 = b.resolveParam("name", emptyLocal, emptyRuntime);
        CHECK(r3.hasValue());
        CHECK(r3.value() == "default_user");
    }

    TEST_CASE("block validate params") {
        Block b("test.block");
        b.setTemplate(Template("Hello, {{name}}!"));

        // Missing param
        auto err = b.validateParams({}, {});
        CHECK(err.isError());
        CHECK(err.code == ErrorCode::MissingParam);

        // With default
        b.setDefaults({{"name", "user"}});
        err = b.validateParams({}, {});
        CHECK(err.isSuccess());
    }

    TEST_CASE("block publish") {
        Block b("test.block");
        b.setTemplate(Template("Hello!"));

        CHECK(b.state() == BlockState::Draft);

        auto err = b.publish(Version{1, 0});
        CHECK(err.isSuccess());
        CHECK(b.state() == BlockState::Published);
        CHECK(b.version() == Version{1, 0});
    }

    TEST_CASE("cannot publish without template") {
        Block b("test.block");
        auto err = b.publish(Version{1, 0});
        CHECK(err.isError());
    }

    TEST_CASE("cannot publish already published block") {
        Block b("test.block");
        b.setTemplate(Template("Hello!"));
        auto err = b.publish(Version{1, 0});
        CHECK(err.isSuccess());

        err = b.publish(Version{2, 0});
        CHECK(err.isError());
    }

    TEST_CASE("block deprecate") {
        Block b("test.block");
        b.setTemplate(Template("Hello!"));
        auto err = b.publish(Version{1, 0});
        CHECK(err.isSuccess());

        b.deprecate();
        CHECK(b.state() == BlockState::Deprecated);
    }
}

// ==================== BlockRef Tests ====================

TEST_SUITE("BlockRef") {
    TEST_CASE("BlockRef with version") {
        BlockRef ref("block.id", Version{1, 5});
        CHECK(ref.blockId() == "block.id");
        CHECK(ref.version().has_value());
        CHECK(ref.version().value() == Version{1, 5});
        CHECK_FALSE(ref.useLatest());
    }

    TEST_CASE("BlockRef with use_latest") {
        BlockRef ref("block.id");
        CHECK(ref.blockId() == "block.id");
        CHECK_FALSE(ref.version().has_value());
        CHECK(ref.useLatest());
    }

    TEST_CASE("BlockRef validate in draft context allows use_latest") {
        BlockRef ref("block.id");  // use_latest = true
        auto err = ref.validate(true);  // isDraftContext = true
        CHECK(err.isSuccess());
    }

    TEST_CASE("BlockRef validate in non-draft context rejects use_latest") {
        BlockRef ref("block.id");  // use_latest = true
        auto err = ref.validate(false);  // isDraftContext = false
        CHECK(err.isError());
        CHECK(err.code == ErrorCode::VersionRequired);
    }

    TEST_CASE("BlockRef with version validates in non-draft context") {
        BlockRef ref("block.id", Version{1, 0});
        auto err = ref.validate(false);
        CHECK(err.isSuccess());
    }

    TEST_CASE("BlockRef resolve params") {
        Block block("test.block");
        block.setTemplate(Template("{{greeting}}, {{name}}!"));
        block.setDefaults({{"greeting", "Hello"}});

        BlockRef ref("test.block", Version{1, 0}, {{"name", "World"}});

        Params runtime{{"greeting", "Hi"}};  // Runtime overrides block defaults
        auto result = ref.resolveParams(block, runtime);

        CHECK(result.hasValue());
        CHECK(result.value()["greeting"] == "Hi");  // Runtime wins
        CHECK(result.value()["name"] == "World");   // Local override used
    }
}

// ==================== Fragment Tests ====================

TEST_SUITE("Fragment") {
    TEST_CASE("Fragment BlockRef type") {
        BlockRef ref("block.id", Version{1, 0});
        Fragment f(Fragment::makeBlockRef(std::move(ref)));

        CHECK(f.isBlockRef());
        CHECK_FALSE(f.isStaticText());
        CHECK_FALSE(f.isSeparator());
        CHECK(f.type() == FragmentType::BlockRef);
    }

    TEST_CASE("Fragment StaticText type") {
        Fragment f = Fragment::makeStaticText("Hello, World!");

        CHECK_FALSE(f.isBlockRef());
        CHECK(f.isStaticText());
        CHECK_FALSE(f.isSeparator());
        CHECK(f.type() == FragmentType::StaticText);
        CHECK(f.asStaticText().text() == "Hello, World!");
    }

    TEST_CASE("Fragment Separator type") {
        Fragment f = Fragment::makeSeparator(SeparatorType::Newline);

        CHECK_FALSE(f.isBlockRef());
        CHECK_FALSE(f.isStaticText());
        CHECK(f.isSeparator());
        CHECK(f.type() == FragmentType::Separator);
        CHECK(f.asSeparator().toString() == "\n");
    }

    TEST_CASE("Separator toString variants") {
        CHECK(Separator(SeparatorType::Newline).toString() == "\n");
        CHECK(Separator(SeparatorType::Paragraph).toString() == "\n\n");
        CHECK(Separator(SeparatorType::Hr).toString() == "\n---\n");
    }

    TEST_CASE("Fragment validate delegates to BlockRef") {
        BlockRef ref("block.id");  // use_latest = true
        Fragment f = Fragment::makeBlockRef(ref);

        auto err = f.validate(true);   // Draft context - OK
        CHECK(err.isSuccess());

        err = f.validate(false);  // Non-draft context - Error
        CHECK(err.isError());
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

        c.addStaticText("Hello");
        c.addSeparator(SeparatorType::Newline);
        c.addBlockRef("block.id", Version{1, 0});

        CHECK(c.fragmentCount() == 3);
        CHECK(c.fragment(0).isStaticText());
        CHECK(c.fragment(1).isSeparator());
        CHECK(c.fragment(2).isBlockRef());
    }

    TEST_CASE("insert and remove fragments") {
        Composition c("test.composition");
        c.addStaticText("First");
        c.addStaticText("Third");

        c.insertFragment(1, Fragment::makeStaticText("Second"));
        CHECK(c.fragmentCount() == 3);
        CHECK(c.fragment(1).asStaticText().text() == "Second");

        c.removeFragment(1);
        CHECK(c.fragmentCount() == 2);
        CHECK(c.fragment(1).asStaticText().text() == "Third");
    }

    TEST_CASE("clear fragments") {
        Composition c("test.composition");
        c.addStaticText("Text");
        c.addBlockRef("block.id", Version{1, 0});

        CHECK(c.fragmentCount() == 2);
        c.clearFragments();
        CHECK(c.fragmentCount() == 0);
    }

    TEST_CASE("composition validate requires id") {
        Composition c;  // No ID
        auto err = c.validate();
        CHECK(err.isError());
    }

    TEST_CASE("composition validate with id succeeds") {
        Composition c("test.composition");
        c.addStaticText("Text");
        auto err = c.validate();
        CHECK(err.isSuccess());
    }

    TEST_CASE("composition publish validates BlockRef versions") {
        Composition c("test.composition");
        c.addBlockRefLatest("block.id");  // use_latest = true

        auto err = c.publish(Version{1, 0});
        CHECK(err.isError());  // Cannot publish with use_latest
        CHECK(err.code == ErrorCode::VersionRequired);
    }

    TEST_CASE("composition publish with explicit versions succeeds") {
        Composition c("test.composition");
        c.addBlockRef("block.id", Version{1, 0});

        auto err = c.publish(Version{1, 0});
        CHECK(err.isSuccess());
        CHECK(c.state() == BlockState::Published);
    }

    TEST_CASE("composition deprecate") {
        Composition c("test.composition");
        auto err = c.publish(Version{1, 0});
        CHECK(err.isSuccess());

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
            .withParam("name", "value")
            .withLanguage("en")
            .withStrictMode(true);

        CHECK(ctx.params["name"] == "value");
        CHECK(ctx.targetLanguage == "en");
        CHECK(ctx.strictMode);
    }
}

// ==================== BlockDraftBuilder Tests ====================

TEST_SUITE("BlockDraftBuilder") {
    TEST_CASE("build block with builder") {
        auto block = BlockDraftBuilder("greeting.hello")
            .withType(BlockType::Role)
            .withTemplate(Template("Hello, {{name}}!"))
            .withDefault("name", "World")
            .withTag("simple")
            .withLanguage("en")
            .withDescription("A simple greeting")
            .build();

        CHECK(block.id() == "greeting.hello");
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
        auto comp = CompositionDraftBuilder("welcome.message")
            .withProjectKey("myproject")
            .withDescription("Welcome message")
            .addStaticText("# Welcome\n\n")
            .addBlockRef("greeting.hello", Version{1, 0}, {{"name", "User"}})
            .addSeparator(SeparatorType::Paragraph)
            .addStaticText("Enjoy your stay!")
            .build();

        CHECK(comp.id() == "welcome.message");
        CHECK(comp.projectKey() == "myproject");
        CHECK(comp.description() == "Welcome message");
        CHECK(comp.fragmentCount() == 4);
    }
}

// ==================== Renderer Tests (using real Engine) ====================

TEST_CASE_FIXTURE(EngineTestFixture, "render single block through engine") {
    // Create and publish block
    createAndPublishBlock("greeting.hello", "Hello, {{name}}!", {{"name", "World"}});

    // Render through engine
    auto result = engine.renderBlock("greeting.hello", Version{1, 0});

    CHECK(result.hasValue());
    CHECK(result.value() == "Hello, World!");
}

TEST_CASE_FIXTURE(EngineTestFixture, "render block with runtime override through engine") {
    createAndPublishBlock("greeting.hello", "Hello, {{name}}!", {{"name", "World"}});

    auto ctx = RenderContext{}.withParam("name", "Alice");
    auto result = engine.renderBlock("greeting.hello", Version{1, 0}, ctx);

    CHECK(result.hasValue());
    CHECK(result.value() == "Hello, Alice!");
}

TEST_CASE_FIXTURE(EngineTestFixture, "render block with missing param fails") {
    createAndPublishBlock("greeting.hello", "Hello, {{name}}!");  // No defaults

    auto result = engine.renderBlock("greeting.hello", Version{1, 0});

    CHECK(result.hasError());
    CHECK(result.error().code == ErrorCode::MissingParam);
}

TEST_CASE_FIXTURE(EngineTestFixture, "render composition with static text only") {
    auto comp = engine.createCompositionDraft("test.comp");
    comp.addStaticText("Hello, World!");
    engine.saveComposition(comp);
    engine.publishComposition("test.comp", Version{1, 0});

    auto result = engine.render("test.comp", Version{1, 0});

    CHECK(result.hasValue());
    CHECK(result.value().text == "Hello, World!");
    CHECK(result.value().compositionId == "test.comp");
}

TEST_CASE_FIXTURE(EngineTestFixture, "render unpublished composition fails") {
    auto comp = engine.createCompositionDraft("test.comp");
    comp.addStaticText("Hello");
    engine.saveComposition(comp);
    // Not published

    auto result = engine.render("test.comp");

    CHECK(result.hasError());
    CHECK(result.error().code == ErrorCode::PublishedRequired);
}

TEST_CASE_FIXTURE(EngineTestFixture, "render composition with separator") {
    auto comp = engine.createCompositionDraft("test.comp");
    comp.addStaticText("Line 1");
    comp.addSeparator(SeparatorType::Newline);
    comp.addStaticText("Line 2");
    engine.saveComposition(comp);
    engine.publishComposition("test.comp", Version{1, 0});

    auto result = engine.render("test.comp", Version{1, 0});

    CHECK(result.hasValue());
    CHECK(result.value().text == "Line 1\nLine 2");
}

TEST_CASE_FIXTURE(EngineTestFixture, "render composition with BlockRef") {
    // Create and publish block
    createAndPublishBlock("greeting.hello", "Hello, {{name}}!", {{"name", "World"}});

    // Create and publish composition with BlockRef
    auto comp = engine.createCompositionDraft("test.comp");
    comp.addBlockRef("greeting.hello", Version{1, 0});
    engine.saveComposition(comp);
    engine.publishComposition("test.comp", Version{1, 0});

    auto result = engine.render("test.comp", Version{1, 0});

    CHECK(result.hasValue());
    CHECK(result.value().text == "Hello, World!");
    CHECK(result.value().blocksUsed.size() == 1);
    CHECK(result.value().blocksUsed[0].first == "greeting.hello");
    CHECK(result.value().blocksUsed[0].second == Version{1, 0});
}

TEST_CASE_FIXTURE(EngineTestFixture, "render composition with BlockRef and runtime params") {
    createAndPublishBlock("greeting.hello", "Hello, {{name}}!", {{"name", "World"}});

    auto comp = engine.createCompositionDraft("test.comp");
    comp.addBlockRef("greeting.hello", Version{1, 0});
    engine.saveComposition(comp);
    engine.publishComposition("test.comp", Version{1, 0});

    auto ctx = RenderContext{}.withParam("name", "Alice");
    auto result = engine.render("test.comp", Version{1, 0}, ctx);

    CHECK(result.hasValue());
    CHECK(result.value().text == "Hello, Alice!");
}

TEST_CASE_FIXTURE(EngineTestFixture, "render composition with multiple fragments") {
    createAndPublishBlock("greeting.hello", "Hello, {{name}}!", {{"name", "World"}});
    createAndPublishBlock("farewell.goodbye", "Goodbye, {{name}}!", {{"name", "Friend"}});

    auto comp = engine.createCompositionDraft("test.comp");
    comp.addBlockRef("greeting.hello", Version{1, 0}, {{"name", "Alice"}});
    comp.addSeparator(SeparatorType::Paragraph);
    comp.addBlockRef("farewell.goodbye", Version{1, 0}, {{"name", "Alice"}});
    engine.saveComposition(comp);
    engine.publishComposition("test.comp", Version{1, 0});

    auto result = engine.render("test.comp", Version{1, 0});

    CHECK(result.hasValue());
    CHECK(result.value().text == "Hello, Alice!\n\nGoodbye, Alice!");
    CHECK(result.value().blocksUsed.size() == 2);
}

TEST_CASE_FIXTURE(EngineTestFixture, "render with missing block fails") {
    // Create composition referencing non-existent block
    auto comp = engine.createCompositionDraft("test.comp");
    comp.addBlockRef("missing.block", Version{1, 0});
    engine.saveComposition(comp);
    engine.publishComposition("test.comp", Version{1, 0});

    auto result = engine.render("test.comp", Version{1, 0});

    CHECK(result.hasError());
    CHECK(result.error().code == ErrorCode::BlockNotFound);
}

TEST_CASE_FIXTURE(EngineTestFixture, "render result contains metadata") {
    auto comp = engine.createCompositionDraft("my.composition");
    comp.addStaticText("Text");
    engine.saveComposition(comp);
    engine.publishComposition("my.composition", Version{2, 5});

    auto result = engine.render("my.composition", Version{2, 5});

    CHECK(result.hasValue());
    CHECK(result.value().compositionId == "my.composition");
    CHECK(result.value().compositionVersion == Version{2, 5});
}

// ==================== Engine Integration Tests ====================

TEST_CASE_FIXTURE(EngineTestFixture, "create and save block") {
    auto block = engine.createBlockDraft("test.block");
    block.setTemplate(Template("Hello, {{name}}!"));
    block.setDefaults({{"name", "World"}});

    auto err = engine.saveBlock(block);
    CHECK(err.isSuccess());

    // Load it back
    auto result = engine.loadBlock("test.block");
    CHECK(result.hasValue());
    CHECK(result.value().id() == "test.block");
    CHECK(result.value().templ().extractParamNames().size() == 1);
}

TEST_CASE_FIXTURE(EngineTestFixture, "publish block workflow") {
    auto block = engine.createBlockDraft("test.block");
    block.setTemplate(Template("Hello!"));
    engine.saveBlock(block);

    auto result = engine.publishBlock("test.block", Version{1, 0});
    CHECK(result.hasValue());
    CHECK(result.value().state() == BlockState::Published);
    CHECK(result.value().version() == Version{1, 0});

    // Check latest version
    auto verResult = engine.getLatestBlockVersion("test.block");
    CHECK(verResult.hasValue());
    CHECK(verResult.value() == Version{1, 0});
}

TEST_CASE_FIXTURE(EngineTestFixture, "list blocks") {
    createAndPublishBlock("block1", "Text 1", {}, Version{1, 0});
    createAndPublishBlock("block2", "Text 2", {}, Version{1, 0});

    auto blocks = engine.listBlocks();
    CHECK(blocks.size() == 2);
}

TEST_CASE_FIXTURE(EngineTestFixture, "create and save composition") {
    auto comp = engine.createCompositionDraft("test.comp");
    comp.addStaticText("Hello");

    auto err = engine.saveComposition(comp);
    CHECK(err.isSuccess());

    auto result = engine.loadComposition("test.comp");
    CHECK(result.hasValue());
    CHECK(result.value().id() == "test.comp");
    CHECK(result.value().fragmentCount() == 1);
}

TEST_CASE_FIXTURE(EngineTestFixture, "publish composition workflow") {
    // First create and publish a block
    createAndPublishBlock("test.block", "Hello!", {}, Version{1, 0});

    // Then create and publish composition referencing it
    auto comp = engine.createCompositionDraft("test.comp");
    comp.addBlockRef("test.block", Version{1, 0});
    engine.saveComposition(comp);

    auto result = engine.publishComposition("test.comp", Version{1, 0});
    CHECK(result.hasValue());
    CHECK(result.value().state() == BlockState::Published);

    // List compositions
    auto comps = engine.listCompositions();
    CHECK(comps.size() == 1);
    CHECK(comps[0] == "test.comp");
}

TEST_CASE_FIXTURE(EngineTestFixture, "validate block through engine") {
    auto block = engine.createBlockDraft("test.block");
    block.setTemplate(Template("Hello, {{name}}!"));
    block.setDefaults({{"name", "World"}});
    engine.saveBlock(block);

    auto err = engine.validateBlock("test.block");
    CHECK(err.isSuccess());
}

TEST_CASE_FIXTURE(EngineTestFixture, "validate composition through engine") {
    createAndPublishBlock("test.block", "Hello!", {}, Version{1, 0});

    auto comp = engine.createCompositionDraft("test.comp");
    comp.addBlockRef("test.block", Version{1, 0});
    engine.saveComposition(comp);

    auto err = engine.validateComposition("test.comp");
    CHECK(err.isSuccess());
}
