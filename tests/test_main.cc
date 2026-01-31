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
        auto block = engine.create_block_draft(id);
        block.set_template(Template(templateStr));
        if (!defaults.empty()) {
            block.set_defaults(defaults);
        }
        auto saveErr = engine.save_block(block);
        REQUIRE(saveErr.is_success());
        auto result = engine.publish_block(id, version);
        REQUIRE(result.has_value());
        return result.value();
    }

    // Helper to create and publish a composition
    Composition createAndPublishComposition(const CompositionId& id,
                                             const std::vector<std::pair<std::string, Version>>& blockRefs,
                                             Version version = Version{1, 0}) {
        auto comp = engine.create_composition_draft(id);
        for (const auto& [blockId, ver] : blockRefs) {
            comp.add_block_ref(blockId, ver);
        }
        auto saveErr = engine.save_composition(comp);
        REQUIRE(saveErr.is_success());
        auto result = engine.publish_composition(id, version);
        REQUIRE(result.has_value());
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
        CHECK(err.is_success());
        CHECK_FALSE(err.is_error());
    }

    TEST_CASE("missing param error") {
        auto err = Error::missing_param("name");
        CHECK(err.is_error());
        CHECK(err.code == ErrorCode::MissingParam);
        CHECK(err.message.find("name") != std::string::npos);
    }

    TEST_CASE("version required error") {
        auto err = Error::version_required();
        CHECK(err.is_error());
        CHECK(err.code == ErrorCode::VersionRequired);
    }
}

// ==================== Result Tests ====================

TEST_SUITE("Result") {
    TEST_CASE("Result with value") {
        Result<int> r(42);
        CHECK(r.has_value());
        CHECK_FALSE(r.has_error());
        CHECK(*r == 42);
        CHECK(r.value() == 42);
    }

    TEST_CASE("Result with error") {
        Result<int> r(Error::missing_param("test"));
        CHECK_FALSE(r.has_value());
        CHECK(r.has_error());
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
        auto params = t.extract_param_names();
        CHECK(params.size() == 2);
        CHECK(params[0] == "name");
        CHECK(params[1] == "score");
    }

    TEST_CASE("expand template with all params") {
        Template t("Hello, {{name}}!");
        Params p{{"name", "World"}};
        auto result = t.expand(p);
        CHECK(result.has_value());
        CHECK(result.value() == "Hello, World!");
    }

    TEST_CASE("expand template with missing param returns error") {
        Template t("Hello, {{name}}!");
        Params p{};
        auto result = t.expand(p);
        CHECK(result.has_error());
        CHECK(result.error().code == ErrorCode::MissingParam);
    }

    TEST_CASE("expand template with no params") {
        Template t("Hello, World!");
        Params p{};
        auto result = t.expand(p);
        CHECK(result.has_value());
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
        b.set_defaults({{"name", "default_user"}});

        Params local{{"name", "local_user"}};
        Params runtime{{"name", "runtime_user"}};

        // Runtime has highest priority
        auto r1 = b.resolve_param("name", local, runtime);
        CHECK(r1.has_value());
        CHECK(r1.value() == "runtime_user");

        // Local has priority over defaults
        Params emptyRuntime;
        auto r2 = b.resolve_param("name", local, emptyRuntime);
        CHECK(r2.has_value());
        CHECK(r2.value() == "local_user");

        // Defaults used when no overrides
        Params emptyLocal;
        auto r3 = b.resolve_param("name", emptyLocal, emptyRuntime);
        CHECK(r3.has_value());
        CHECK(r3.value() == "default_user");
    }

    TEST_CASE("block validate params") {
        Block b("test.block");
        b.set_template(Template("Hello, {{name}}!"));

        // Missing param
        auto err = b.validate_params({}, {});
        CHECK(err.is_error());
        CHECK(err.code == ErrorCode::MissingParam);

        // With default
        b.set_defaults({{"name", "user"}});
        err = b.validate_params({}, {});
        CHECK(err.is_success());
    }

    TEST_CASE("block publish") {
        Block b("test.block");
        b.set_template(Template("Hello!"));

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
        b.set_template(Template("Hello!"));
        auto err = b.publish(Version{1, 0});
        CHECK(err.is_success());

        err = b.publish(Version{2, 0});
        CHECK(err.is_error());
    }

    TEST_CASE("block deprecate") {
        Block b("test.block");
        b.set_template(Template("Hello!"));
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
        CHECK(ref.block_id() == "block.id");
        CHECK(ref.version().has_value());
        CHECK(ref.version().value() == Version{1, 5});
        CHECK_FALSE(ref.use_latest());
    }

    TEST_CASE("BlockRef with use_latest") {
        BlockRef ref("block.id");
        CHECK(ref.block_id() == "block.id");
        CHECK_FALSE(ref.version().has_value());
        CHECK(ref.use_latest());
    }

    TEST_CASE("BlockRef validate in draft context allows use_latest") {
        BlockRef ref("block.id");  // use_latest = true
        auto err = ref.validate(true);  // isDraftContext = true
        CHECK(err.is_success());
    }

    TEST_CASE("BlockRef validate in non-draft context rejects use_latest") {
        BlockRef ref("block.id");  // use_latest = true
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
        block.set_template(Template("{{greeting}}, {{name}}!"));
        block.set_defaults({{"greeting", "Hello"}});

        BlockRef ref("test.block", Version{1, 0}, {{"name", "World"}});

        Params runtime{{"greeting", "Hi"}};  // Runtime overrides block defaults
        auto result = ref.resolve_params(block, runtime);

        CHECK(result.has_value());
        CHECK(result.value()["greeting"] == "Hi");  // Runtime wins
        CHECK(result.value()["name"] == "World");   // Local override used
    }
}

// ==================== Fragment Tests ====================

TEST_SUITE("Fragment") {
    TEST_CASE("Fragment BlockRef type") {
        BlockRef ref("block.id", Version{1, 0});
        Fragment f(Fragment::make_block_ref(std::move(ref)));

        CHECK(f.is_block_ref());
        CHECK_FALSE(f.is_static_text());
        CHECK_FALSE(f.is_separator());
        CHECK(f.type() == FragmentType::BlockRef);
    }

    TEST_CASE("Fragment StaticText type") {
        Fragment f = Fragment::make_static_text("Hello, World!");

        CHECK_FALSE(f.is_block_ref());
        CHECK(f.is_static_text());
        CHECK_FALSE(f.is_separator());
        CHECK(f.type() == FragmentType::StaticText);
        CHECK(f.as_static_text().text() == "Hello, World!");
    }

    TEST_CASE("Fragment Separator type") {
        Fragment f = Fragment::make_separator(SeparatorType::Newline);

        CHECK_FALSE(f.is_block_ref());
        CHECK_FALSE(f.is_static_text());
        CHECK(f.is_separator());
        CHECK(f.type() == FragmentType::Separator);
        CHECK(f.as_separator().toString() == "\n");
    }

    TEST_CASE("Separator toString variants") {
        CHECK(Separator(SeparatorType::Newline).toString() == "\n");
        CHECK(Separator(SeparatorType::Paragraph).toString() == "\n\n");
        CHECK(Separator(SeparatorType::Hr).toString() == "\n---\n");
    }

    TEST_CASE("Fragment validate delegates to BlockRef") {
        BlockRef ref("block.id");  // use_latest = true
        Fragment f = Fragment::make_block_ref(ref);

        auto err = f.validate(true);   // Draft context - OK
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

        c.add_static_text("Hello");
        c.add_separator(SeparatorType::Newline);
        c.add_block_ref("block.id", Version{1, 0});

        CHECK(c.fragmentCount() == 3);
        CHECK(c.fragment(0).is_static_text());
        CHECK(c.fragment(1).is_separator());
        CHECK(c.fragment(2).is_block_ref());
    }

    TEST_CASE("insert and remove fragments") {
        Composition c("test.composition");
        c.add_static_text("First");
        c.add_static_text("Third");

        c.insert_fragment(1, Fragment::make_static_text("Second"));
        CHECK(c.fragmentCount() == 3);
        CHECK(c.fragment(1).as_static_text().text() == "Second");

        c.remove_fragment(1);
        CHECK(c.fragmentCount() == 2);
        CHECK(c.fragment(1).as_static_text().text() == "Third");
    }

    TEST_CASE("clear fragments") {
        Composition c("test.composition");
        c.add_static_text("Text");
        c.add_block_ref("block.id", Version{1, 0});

        CHECK(c.fragmentCount() == 2);
        c.clear_fragments();
        CHECK(c.fragmentCount() == 0);
    }

    TEST_CASE("composition validate requires id") {
        Composition c;  // No ID
        auto err = c.validate();
        CHECK(err.is_error());
    }

    TEST_CASE("composition validate with id succeeds") {
        Composition c("test.composition");
        c.add_static_text("Text");
        auto err = c.validate();
        CHECK(err.is_success());
    }

    TEST_CASE("composition publish validates BlockRef versions") {
        Composition c("test.composition");
        c.add_block_ref_latest("block.id");  // use_latest = true

        auto err = c.publish(Version{1, 0});
        CHECK(err.is_error());  // Cannot publish with use_latest
        CHECK(err.code == ErrorCode::VersionRequired);
    }

    TEST_CASE("composition publish with explicit versions succeeds") {
        Composition c("test.composition");
        c.add_block_ref("block.id", Version{1, 0});

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
            .with_param("name", "value")
            .with_language("en")
            .with_strict_mode(true);

        CHECK(ctx.params["name"] == "value");
        CHECK(ctx.targetLanguage == "en");
        CHECK(ctx.strictMode);
    }
}

// ==================== BlockDraftBuilder Tests ====================

TEST_SUITE("BlockDraftBuilder") {
    TEST_CASE("build block with builder") {
        auto block = BlockDraftBuilder("greeting.hello")
            .with_type(BlockType::Role)
            .with_template(Template("Hello, {{name}}!"))
            .with_default("name", "World")
            .with_tag("simple")
            .with_language("en")
            .with_description("A simple greeting")
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
            .with_project_key("myproject")
            .with_description("Welcome message")
            .add_static_text("# Welcome\n\n")
            .add_block_ref("greeting.hello", Version{1, 0}, {{"name", "User"}})
            .add_separator(SeparatorType::Paragraph)
            .add_static_text("Enjoy your stay!")
            .build();

        CHECK(comp.id() == "welcome.message");
        CHECK(comp.project_key() == "myproject");
        CHECK(comp.description() == "Welcome message");
        CHECK(comp.fragmentCount() == 4);
    }
}

// ==================== Renderer Tests (using real Engine) ====================

TEST_CASE_FIXTURE(EngineTestFixture, "render single block through engine") {
    // Create and publish block
    createAndPublishBlock("greeting.hello", "Hello, {{name}}!", {{"name", "World"}});

    // Render through engine
    auto result = engine.render_block("greeting.hello", Version{1, 0});

    CHECK(result.has_value());
    CHECK(result.value() == "Hello, World!");
}

TEST_CASE_FIXTURE(EngineTestFixture, "render block with runtime override through engine") {
    createAndPublishBlock("greeting.hello", "Hello, {{name}}!", {{"name", "World"}});

    auto ctx = RenderContext{}.with_param("name", "Alice");
    auto result = engine.render_block("greeting.hello", Version{1, 0}, ctx);

    CHECK(result.has_value());
    CHECK(result.value() == "Hello, Alice!");
}

TEST_CASE_FIXTURE(EngineTestFixture, "render block with missing param fails") {
    createAndPublishBlock("greeting.hello", "Hello, {{name}}!");  // No defaults

    auto result = engine.render_block("greeting.hello", Version{1, 0});

    CHECK(result.has_error());
    CHECK(result.error().code == ErrorCode::MissingParam);
}

TEST_CASE_FIXTURE(EngineTestFixture, "render composition with static text only") {
    auto comp = engine.create_composition_draft("test.comp");
    comp.add_static_text("Hello, World!");
    engine.save_composition(comp);
    engine.publish_composition("test.comp", Version{1, 0});

    auto result = engine.render("test.comp", Version{1, 0});

    CHECK(result.has_value());
    CHECK(result.value().text == "Hello, World!");
    CHECK(result.value().compositionId == "test.comp");
}

TEST_CASE_FIXTURE(EngineTestFixture, "render unpublished composition fails") {
    auto comp = engine.create_composition_draft("test.comp");
    comp.add_static_text("Hello");
    engine.save_composition(comp);
    // Not published

    auto result = engine.render("test.comp");

    CHECK(result.has_error());
    CHECK(result.error().code == ErrorCode::PublishedRequired);
}

TEST_CASE_FIXTURE(EngineTestFixture, "render composition with separator") {
    auto comp = engine.create_composition_draft("test.comp");
    comp.add_static_text("Line 1");
    comp.add_separator(SeparatorType::Newline);
    comp.add_static_text("Line 2");
    engine.save_composition(comp);
    engine.publish_composition("test.comp", Version{1, 0});

    auto result = engine.render("test.comp", Version{1, 0});

    CHECK(result.has_value());
    CHECK(result.value().text == "Line 1\nLine 2");
}

TEST_CASE_FIXTURE(EngineTestFixture, "render composition with BlockRef") {
    // Create and publish block
    createAndPublishBlock("greeting.hello", "Hello, {{name}}!", {{"name", "World"}});

    // Create and publish composition with BlockRef
    auto comp = engine.create_composition_draft("test.comp");
    comp.add_block_ref("greeting.hello", Version{1, 0});
    engine.save_composition(comp);
    engine.publish_composition("test.comp", Version{1, 0});

    auto result = engine.render("test.comp", Version{1, 0});

    CHECK(result.has_value());
    CHECK(result.value().text == "Hello, World!");
    CHECK(result.value().blocksUsed.size() == 1);
    CHECK(result.value().blocksUsed[0].first == "greeting.hello");
    CHECK(result.value().blocksUsed[0].second == Version{1, 0});
}

TEST_CASE_FIXTURE(EngineTestFixture, "render composition with BlockRef and runtime params") {
    createAndPublishBlock("greeting.hello", "Hello, {{name}}!", {{"name", "World"}});

    auto comp = engine.create_composition_draft("test.comp");
    comp.add_block_ref("greeting.hello", Version{1, 0});
    engine.save_composition(comp);
    engine.publish_composition("test.comp", Version{1, 0});

    auto ctx = RenderContext{}.with_param("name", "Alice");
    auto result = engine.render("test.comp", Version{1, 0}, ctx);

    CHECK(result.has_value());
    CHECK(result.value().text == "Hello, Alice!");
}

TEST_CASE_FIXTURE(EngineTestFixture, "render composition with multiple fragments") {
    createAndPublishBlock("greeting.hello", "Hello, {{name}}!", {{"name", "World"}});
    createAndPublishBlock("farewell.goodbye", "Goodbye, {{name}}!", {{"name", "Friend"}});

    auto comp = engine.create_composition_draft("test.comp");
    comp.add_block_ref("greeting.hello", Version{1, 0}, {{"name", "Alice"}});
    comp.add_separator(SeparatorType::Paragraph);
    comp.add_block_ref("farewell.goodbye", Version{1, 0}, {{"name", "Alice"}});
    engine.save_composition(comp);
    engine.publish_composition("test.comp", Version{1, 0});

    auto result = engine.render("test.comp", Version{1, 0});

    CHECK(result.has_value());
    CHECK(result.value().text == "Hello, Alice!\n\nGoodbye, Alice!");
    CHECK(result.value().blocksUsed.size() == 2);
}

TEST_CASE_FIXTURE(EngineTestFixture, "render with missing block fails") {
    // Create composition referencing non-existent block
    auto comp = engine.create_composition_draft("test.comp");
    comp.add_block_ref("missing.block", Version{1, 0});
    engine.save_composition(comp);
    engine.publish_composition("test.comp", Version{1, 0});

    auto result = engine.render("test.comp", Version{1, 0});

    CHECK(result.has_error());
    CHECK(result.error().code == ErrorCode::BlockNotFound);
}

TEST_CASE_FIXTURE(EngineTestFixture, "render result contains metadata") {
    auto comp = engine.create_composition_draft("my.composition");
    comp.add_static_text("Text");
    engine.save_composition(comp);
    engine.publish_composition("my.composition", Version{2, 5});

    auto result = engine.render("my.composition", Version{2, 5});

    CHECK(result.has_value());
    CHECK(result.value().compositionId == "my.composition");
    CHECK(result.value().compositionVersion == Version{2, 5});
}

// ==================== Engine Integration Tests ====================

TEST_CASE_FIXTURE(EngineTestFixture, "create and save block") {
    auto block = engine.create_block_draft("test.block");
    block.set_template(Template("Hello, {{name}}!"));
    block.set_defaults({{"name", "World"}});

    auto err = engine.save_block(block);
    CHECK(err.is_success());

    // Load it back
    auto result = engine.load_block("test.block");
    CHECK(result.has_value());
    CHECK(result.value().id() == "test.block");
    CHECK(result.value().templ().extract_param_names().size() == 1);
}

TEST_CASE_FIXTURE(EngineTestFixture, "publish block workflow") {
    auto block = engine.create_block_draft("test.block");
    block.set_template(Template("Hello!"));
    engine.save_block(block);

    auto result = engine.publish_block("test.block", Version{1, 0});
    CHECK(result.has_value());
    CHECK(result.value().state() == BlockState::Published);
    CHECK(result.value().version() == Version{1, 0});

    // Check latest version
    auto verResult = engine.get_latest_block_version("test.block");
    CHECK(verResult.has_value());
    CHECK(verResult.value() == Version{1, 0});
}

TEST_CASE_FIXTURE(EngineTestFixture, "list blocks") {
    createAndPublishBlock("block1", "Text 1", {}, Version{1, 0});
    createAndPublishBlock("block2", "Text 2", {}, Version{1, 0});

    auto blocks = engine.list_blocks();
    CHECK(blocks.size() == 2);
}

TEST_CASE_FIXTURE(EngineTestFixture, "create and save composition") {
    auto comp = engine.create_composition_draft("test.comp");
    comp.add_static_text("Hello");

    auto err = engine.save_composition(comp);
    CHECK(err.is_success());

    auto result = engine.load_composition("test.comp");
    CHECK(result.has_value());
    CHECK(result.value().id() == "test.comp");
    CHECK(result.value().fragmentCount() == 1);
}

TEST_CASE_FIXTURE(EngineTestFixture, "publish composition workflow") {
    // First create and publish a block
    createAndPublishBlock("test.block", "Hello!", {}, Version{1, 0});

    // Then create and publish composition referencing it
    auto comp = engine.create_composition_draft("test.comp");
    comp.add_block_ref("test.block", Version{1, 0});
    engine.save_composition(comp);

    auto result = engine.publish_composition("test.comp", Version{1, 0});
    CHECK(result.has_value());
    CHECK(result.value().state() == BlockState::Published);

    // List compositions
    auto comps = engine.list_compositions();
    CHECK(comps.size() == 1);
    CHECK(comps[0] == "test.comp");
}

TEST_CASE_FIXTURE(EngineTestFixture, "validate block through engine") {
    auto block = engine.create_block_draft("test.block");
    block.set_template(Template("Hello, {{name}}!"));
    block.set_defaults({{"name", "World"}});
    engine.save_block(block);

    auto err = engine.validate_block("test.block");
    CHECK(err.is_success());
}

TEST_CASE_FIXTURE(EngineTestFixture, "validate composition through engine") {
    createAndPublishBlock("test.block", "Hello!", {}, Version{1, 0});

    auto comp = engine.create_composition_draft("test.comp");
    comp.add_block_ref("test.block", Version{1, 0});
    engine.save_composition(comp);

    auto err = engine.validate_composition("test.comp");
    CHECK(err.is_success());
}
