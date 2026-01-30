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

using namespace tf;

// Hash function for pair<BlockId, Version>
struct PairHash {
    size_t operator()(const std::pair<BlockId, Version>& p) const {
        return std::hash<BlockId>{}(p.first) ^
               (std::hash<uint16_t>{}(p.second.major) << 1) ^
               (std::hash<uint16_t>{}(p.second.minor) << 2);
    }
};

// Mock block cache for testing
class MockBlockCache : public IBlockCache {
public:
    void addBlock(Block block) {
        blocks_[{block.id(), block.version()}] = std::move(block);
    }

    const Block* getBlock(const BlockId& id, Version version) const override {
        auto it = blocks_.find({id, version});
        if (it != blocks_.end()) {
            return &it->second;
        }
        return nullptr;
    }

    const Block* getLatestBlock(const BlockId& id) const override {
        const Block* latest = nullptr;
        for (const auto& [key, block] : blocks_) {
            if (key.first == id && (!latest || block.version() > latest->version())) {
                latest = &block;
            }
        }
        return latest;
    }

private:
    mutable std::unordered_map<std::pair<BlockId, Version>, Block, PairHash> blocks_;
};

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
        b.publish(Version{1, 0});

        auto err = b.publish(Version{2, 0});
        CHECK(err.isError());
    }

    TEST_CASE("block deprecate") {
        Block b("test.block");
        b.setTemplate(Template("Hello!"));
        b.publish(Version{1, 0});

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
        BlockRef ref("block.id");
        auto err = ref.validate(true);  // isDraftContext = true
        CHECK(err.isSuccess());
    }

    TEST_CASE("BlockRef validate in non-draft context rejects use_latest") {
        BlockRef ref("block.id");
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
        c.publish(Version{1, 0});

        c.deprecate();
        CHECK(c.state() == BlockState::Deprecated);
    }

    TEST_CASE("StructuralStyle factories") {
        auto plain = StructuralStyle::plain();
        CHECK(plain.outputFormat == StructuralStyle::OutputFormat::Plain);

        auto markdown = StructuralStyle::markdown();
        CHECK(markdown.outputFormat == StructuralStyle::OutputFormat::Markdown);

        auto json = StructuralStyle::json();
        CHECK(json.outputFormat == StructuralStyle::OutputFormat::Json);
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

// ==================== Renderer Tests ====================

TEST_SUITE("Renderer") {
    TEST_CASE("render single block") {
        Block b("greeting.hello");
        b.setTemplate(Template("Hello, {{name}}!"));
        b.setDefaults({{"name", "World"}});

        Renderer renderer;
        auto result = renderer.renderBlock(b, RenderContext{});

        CHECK(result.hasValue());
        CHECK(result.value() == "Hello, World!");
    }

    TEST_CASE("render block with runtime override") {
        Block b("greeting.hello");
        b.setTemplate(Template("Hello, {{name}}!"));
        b.setDefaults({{"name", "World"}});

        Renderer renderer;
        auto ctx = RenderContext{}.withParam("name", "Alice");
        auto result = renderer.renderBlock(b, ctx);

        CHECK(result.hasValue());
        CHECK(result.value() == "Hello, Alice!");
    }

    TEST_CASE("render block with missing param fails") {
        Block b("greeting.hello");
        b.setTemplate(Template("Hello, {{name}}!"));
        // No defaults

        Renderer renderer;
        auto result = renderer.renderBlock(b, RenderContext{});

        CHECK(result.hasError());
        CHECK(result.error().code == ErrorCode::MissingParam);
    }

    TEST_CASE("render composition with static text only") {
        Composition c("test.comp");
        c.addStaticText("Hello, World!");
        c.publish(Version{1, 0});

        Renderer renderer;
        auto result = renderer.render(c, RenderContext{});

        CHECK(result.hasValue());
        CHECK(result.value().text == "Hello, World!");
        CHECK(result.value().compositionId == "test.comp");
    }

    TEST_CASE("render unpublished composition fails") {
        Composition c("test.comp");
        c.addStaticText("Hello");
        // Not published

        Renderer renderer;
        auto result = renderer.render(c, RenderContext{});

        CHECK(result.hasError());
        CHECK(result.error().code == ErrorCode::PublishedRequired);
    }

    TEST_CASE("render composition with separator") {
        Composition c("test.comp");
        c.addStaticText("Line 1");
        c.addSeparator(SeparatorType::Newline);
        c.addStaticText("Line 2");
        c.publish(Version{1, 0});

        Renderer renderer;
        auto result = renderer.render(c, RenderContext{});

        CHECK(result.hasValue());
        CHECK(result.value().text == "Line 1\nLine 2");
    }

    TEST_CASE("render composition with BlockRef") {
        // Setup block cache
        std::unique_ptr<IBlockCache> cache = std::make_unique<MockBlockCache>();
        auto* mockCache = dynamic_cast<MockBlockCache*>(cache.get());

        Block block("greeting.hello");
        block.setTemplate(Template("Hello, {{name}}!"));
        block.setDefaults({{"name", "World"}});
        block.publish(Version{1, 0});
        mockCache->addBlock(block);

        // Create composition
        Composition c("test.comp");
        c.addBlockRef("greeting.hello", Version{1, 0});
        c.publish(Version{1, 0});

        Renderer renderer(std::move(cache));
        auto result = renderer.render(c, RenderContext{});

        CHECK(result.hasValue());
        CHECK(result.value().text == "Hello, World!");
        CHECK(result.value().blocksUsed.size() == 1);
        CHECK(result.value().blocksUsed[0].first == "greeting.hello");
        CHECK(result.value().blocksUsed[0].second == Version{1, 0});
    }

    TEST_CASE("render composition with BlockRef and runtime params") {
        std::unique_ptr<IBlockCache> cache = std::make_unique<MockBlockCache>();
        auto* mockCache = static_cast<MockBlockCache*>(cache.get());

        Block block("greeting.hello");
        block.setTemplate(Template("Hello, {{name}}!"));
        block.setDefaults({{"name", "World"}});
        block.publish(Version{1, 0});
        mockCache->addBlock(block);

        Composition c("test.comp");
        c.addBlockRef("greeting.hello", Version{1, 0});
        c.publish(Version{1, 0});

        Renderer renderer(std::move(cache));
        auto ctx = RenderContext{}.withParam("name", "Alice");
        auto result = renderer.render(c, ctx);

        CHECK(result.hasValue());
        CHECK(result.value().text == "Hello, Alice!");
    }

    TEST_CASE("render composition with multiple fragments") {
        std::unique_ptr<IBlockCache> cache = std::make_unique<MockBlockCache>();
        auto* mockCache = static_cast<MockBlockCache*>(cache.get());

        Block greeting("greeting.hello");
        greeting.setTemplate(Template("Hello, {{name}}!"));
        greeting.setDefaults({{"name", "World"}});
        greeting.publish(Version{1, 0});
        mockCache->addBlock(greeting);

        Block farewell("farewell.goodbye");
        farewell.setTemplate(Template("Goodbye, {{name}}!"));
        farewell.setDefaults({{"name", "Friend"}});
        farewell.publish(Version{1, 0});
        mockCache->addBlock(farewell);

        Composition c("test.comp");
        c.addBlockRef("greeting.hello", Version{1, 0}, {{"name", "Alice"}});
        c.addSeparator(SeparatorType::Paragraph);
        c.addBlockRef("farewell.goodbye", Version{1, 0}, {{"name", "Alice"}});
        c.publish(Version{1, 0});

        Renderer renderer(std::move(cache));
        auto result = renderer.render(c, RenderContext{});

        CHECK(result.hasValue());
        CHECK(result.value().text == "Hello, Alice!\n\nGoodbye, Alice!");
        CHECK(result.value().blocksUsed.size() == 2);
    }

    TEST_CASE("render with missing block fails") {
        std::unique_ptr<IBlockCache> cache = std::make_unique<MockBlockCache>();
        // No blocks added

        Composition c("test.comp");
        c.addBlockRef("missing.block", Version{1, 0});
        c.publish(Version{1, 0});

        Renderer renderer(std::move(cache));
        auto result = renderer.render(c, RenderContext{});

        CHECK(result.hasError());
        CHECK(result.error().code == ErrorCode::BlockNotFound);
    }

    TEST_CASE("apply structural style with wrapper") {
        Renderer renderer;

        StructuralStyle style;
        style.blockWrapper = "<div>{{content}}</div>";

        std::vector<std::string> fragments{"Hello", "World"};
        auto result = renderer.applyStructuralStyle(fragments, style);

        CHECK(result == "<div>Hello</div><div>World</div>");
    }

    TEST_CASE("apply structural style with delimiter") {
        Renderer renderer;

        StructuralStyle style;
        style.delimiter = ", ";

        std::vector<std::string> fragments{"A", "B", "C"};
        auto result = renderer.applyStructuralStyle(fragments, style);

        CHECK(result == "A, B, C");
    }

    TEST_CASE("apply structural style with preamble and postamble") {
        Renderer renderer;

        StructuralStyle style;
        style.preamble = "START:";
        style.postamble = ":END";

        std::vector<std::string> fragments{"Content"};
        auto result = renderer.applyStructuralStyle(fragments, style);

        CHECK(result == "START:Content:END");
    }

    TEST_CASE("render result contains metadata") {
        Composition c("my.composition");
        c.addStaticText("Text");
        c.publish(Version{2, 5});

        Renderer renderer;
        auto result = renderer.render(c, RenderContext{});

        CHECK(result.hasValue());
        CHECK(result.value().compositionId == "my.composition");
        CHECK(result.value().compositionVersion == Version{2, 5});
        CHECK(result.value().format == StructuralStyle::OutputFormat::Plain);
    }
}
