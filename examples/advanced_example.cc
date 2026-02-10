#include <tf/engine.h>

#include <format>
#include <iostream>
#include <vector>

using namespace tf;

void print_render_result(const Result<RenderResult>& result) {
  if (result.HasError()) {
    std::cerr << std::format("❌ Error [{}]: {}\n",
                             static_cast<int>(result.error().code),
                             result.error().message);
    return;
  }
  const auto& rr = result.value();
  std::cout << std::format(
      "✅ Success | Composition: {} v{}.{} | Blocks used: {}\n",
      rr.compositionId, rr.compositionVersion.major,
      rr.compositionVersion.minor, rr.blocksUsed.size());
  std::cout << std::format("Text ({} chars):\n{}\n", rr.text.length(), rr.text);
  std::cout << "------------------------------------------------\n\n";
}

int main() {
  try {
    EngineConfig config{.ProjectKey = "demo",
                        .strict_mode = true,
                        .default_data_path = "memory:test_app"};
    Engine engine(config);

    std::cout
        << "=== Advanced TextFoundry Example: LLM Code Review Prompt ===\n\n";

    // Publish 12+ blocks of different types
    std::cout << "📦 Publishing blocks...\n";

    // 1. Meta header
    auto header_draft = BlockDraftBuilder("meta.header")
                            .WithType(BlockType::Meta)
                            .WithTemplate(Template(R"(# {{title}}
**Author:** {{author}} | **Date:** {{date}} | **Audience:** {{audience}})"))
                            .WithDefault("author", "Engineering Team")
                            .WithDefault("date", "2026-02-02")
                            .WithDefault("audience", "developers")
                            .WithDescription("Document header with metadata")
                            .build();
    auto header_pub =
        engine.PublishBlock(std::move(header_draft), Version{1, 0});
    if (!header_pub.HasValue()) {
      std::cerr << "Header error: " << header_pub.error().message << "\n";
      return 1;
    }
    Version header_version = header_pub.value().version();

    // 2. Role system
    auto system_draft =
        BlockDraftBuilder("role.system")
            .WithType(BlockType::Role)
            .WithTemplate(Template(
                R"(You are a {{role_type}} assistant specialized in {{domain}}. 
Follow all instructions precisely. )"))
            .WithDefault("role_type", "highly competent")
            .WithDefault("domain", "software engineering")
            .WithDescription("System role definition")
            .build();
    auto system_pub =
        engine.PublishBlock(std::move(system_draft), Version{1, 0});
    Version system_version = system_pub.value().version();

    // 3. Constraint chain-of-thought
    auto cot_draft =
        BlockDraftBuilder("constraint.cot")
            .WithType(BlockType::Constraint)
            .WithTemplate(Template(R"(> **Think step-by-step:**
> 1. Analyze structure
> 2. Check {{checkpoints}}
> 3. Suggest improvements
> 4. Output in {{format}})"))
            .WithDefault("checkpoints", "security, performance, readability")
            .WithDefault("format", "JSON")
            .WithDescription("Chain of thought instruction")
            .build();
    auto cot_pub = engine.PublishBlock(std::move(cot_draft), Version{1, 0});
    Version cot_version = cot_pub.value().version();

    // 4. Constraint JSON output
    auto json_draft =
        BlockDraftBuilder("constraint.json")
            .WithType(BlockType::Constraint)
            .WithTemplate(Template(
                R"(**Output format:** Valid JSON only matching this schema:
```json
{
  "issues": [{{schema}}],
  "score": {{score_range}},
  "recommendations": "{{rec}}"
}
```)"))
            .WithDefault("schema", "array of issue objects")
            .WithDefault("score_range", "0-10")
            .WithDefault("rec", "actionable steps")
            .WithDescription("JSON output constraint")
            .build();
    auto json_pub = engine.PublishBlock(std::move(json_draft), Version{1, 0});
    Version json_version = json_pub.value().version();

    // 5. Domain project context
    auto project_draft = BlockDraftBuilder("domain.project")
                             .WithType(BlockType::Domain)
                             .WithTemplate(Template(R"(## Project Context
**Name:** {{name}}
**Stack:** {{stack}}
**Goals:** {{goals}}
**Key features:** {{features}})"))
                             .WithDefault("name", "Unnamed Project")
                             .WithDefault("stack", "C++")
                             .WithDefault("goals", "high performance")
                             .WithDefault("features", "text templating")
                             .WithDescription("Project overview")
                             .build();
    auto project_pub =
        engine.PublishBlock(std::move(project_draft), Version{1, 0});
    Version project_version = project_pub.value().version();

    // 6. Role code reviewer
    auto reviewer_draft =
        BlockDraftBuilder("role.reviewer")
            .WithType(BlockType::Role)
            .WithTemplate(Template(
                R"(Act as a senior {{level}} code reviewer with 15+ years experience.
Review for: {{criteria}}. 
Be constructive and precise.)"))
            .WithDefault("level", "software engineer")
            .WithDefault("criteria", "correctness, style, security")
            .WithDescription("Code reviewer persona")
            .build();
    auto reviewer_pub =
        engine.PublishBlock(std::move(reviewer_draft), Version{1, 0});
    Version reviewer_version = reviewer_pub.value().version();

    // 7. Constraint security
    auto security_draft = BlockDraftBuilder("constraint.security")
                              .WithType(BlockType::Constraint)
                              .WithTemplate(Template(R"(> ⚠️ **Security Check:**
> - SQL/NoSQL injection
> - XSS/CSRF
> - Auth bypass
> - {{custom_checks}}
> Flag ALL potential issues!)"))
                              .WithDefault("custom_checks", "data validation")
                              .WithDescription("Security constraints")
                              .build();
    auto security_pub =
        engine.PublishBlock(std::move(security_draft), Version{1, 0});
    Version security_version = security_pub.value().version();

    // 8. Domain coding guidelines
    auto guidelines_draft =
        BlockDraftBuilder("domain.guidelines")
            .WithType(BlockType::Domain)
            .WithTemplate(Template(R"(## Coding Guidelines
- Use {{style_guide}}
- Complexity < {{max_complexity}}
- Tests coverage > {{min_coverage}}%
- {{additional_rules}})"))
            .WithDefault("style_guide", "Google C++ Style")
            .WithDefault("max_complexity", "10")
            .WithDefault("min_coverage", "80")
            .WithDefault("additional_rules", "No raw pointers")
            .WithDescription("Coding standards")
            .build();
    auto guidelines_pub =
        engine.PublishBlock(std::move(guidelines_draft), Version{1, 0});
    Version guidelines_version = guidelines_pub.value().version();

    // 9. Examples bad code
    auto bad_ex_draft = BlockDraftBuilder("examples.bad")
                            .WithType(BlockType::Domain)
                            .WithTemplate(Template(R"(## Bad Example
```cpp
{{code}}
```
**Issues:** {{issues}})"))
                            .WithDefault("issues", "memory leak, no validation")
                            .WithDescription("Bad code example")
                            .build();
    auto bad_ex_pub =
        engine.PublishBlock(std::move(bad_ex_draft), Version{1, 0});
    Version bad_ex_version = bad_ex_pub.value().version();

    // 10. Examples good code
    auto good_ex_draft = BlockDraftBuilder("examples.good")
                             .WithType(BlockType::Domain)
                             .WithTemplate(Template(R"(## Good Example
```cpp
{{code}}
```
**Why good:** {{why_good}})"))
                             .WithDefault("why_good", "RAII, validation, tests")
                             .WithDescription("Good code example")
                             .build();
    auto good_ex_pub =
        engine.PublishBlock(std::move(good_ex_draft), Version{1, 0});
    Version good_ex_version = good_ex_pub.value().version();

    // 11. Constraint performance
    auto perf_draft = BlockDraftBuilder("constraint.performance")
                          .WithType(BlockType::Constraint)
                          .WithTemplate(Template(R"(> **Performance Review:**
> - Time: O({{time_complexity}})
> - Space: O({{space_complexity}})
> - Bottlenecks: {{bottlenecks}})"))
                          .WithDefault("time_complexity", "n log n")
                          .WithDefault("space_complexity", "n")
                          .WithDefault("bottlenecks", "none")
                          .WithDescription("Performance constraints")
                          .build();
    auto perf_pub = engine.PublishBlock(std::move(perf_draft), Version{1, 0});
    Version perf_version = perf_pub.value().version();

    // 12. Meta footer
    auto footer_draft = BlockDraftBuilder("meta.footer")
                            .WithType(BlockType::Meta)
                            .WithTemplate(Template(R"(---
**Generated:** {{date}} | **Version:** {{version}}
**Blocks used:** {{block_count}})"))
                            .WithDefault("date", "2026-02-02")
                            .WithDefault("version", "1.0")
                            .WithDefault("block_count", "12")
                            .WithDescription("Document footer")
                            .build();
    auto footer_pub =
        engine.PublishBlock(std::move(footer_draft), Version{1, 0});
    if (!footer_pub.HasValue()) {
      std::cerr << "Footer error: " << footer_pub.error().message << "\n";
      return 1;
    }
    Version footer_version = footer_pub.value().version();

    std::cout << "✅ All 12 blocks published\n\n";

    // Build massive composition using ALL blocks + static text + separators
    std::cout << "📝 Building advanced composition...\n";

    StructuralStyle structural{
        .blockWrapper = "BLOCK[{{BlockId}}]:\\n{{content}}\\n\\n",
        .preamble =
            "Full LLM Code Review Prompt (TextFoundry Advanced Demo)\n\n",
        .delimiter = "\\n---\\n"};
    StyleProfile style{.structural = structural};

    auto comp_builder =
        CompositionDraftBuilder("advanced.code_review_prompt")
            .WithProjectKey("demo")
            .WithDescription(
                "Advanced LLM code review prompt using 12 reusable blocks")
            .WithStyleProfile(style)

            // Header
            .AddBlockRef(BlockRef(
                "meta.header", header_version,
                {{"title", "Advanced Code Review for TextFoundry Engine"},
                 {"audience", "senior developers"}}))
            .AddSeparator(SeparatorType::Paragraph)

            // Static intro
            .AddStaticText(R"(## Task
Review the provided C++ code for a templating engine.)")

            // System role
            .AddBlockRef(BlockRef("role.system", system_version,
                                  {{"role_type", "world-class"},
                                   {"domain", "C++ software architecture"}}))
            .AddSeparator(SeparatorType::Newline)

            // Reviewer role
            .AddBlockRef(BlockRef(
                "role.reviewer", reviewer_version,
                {{"level", "staff engineer"},
                 {"criteria",
                  "architecture, security, performance, maintainability"}}))

            // Project context
            .AddBlockRef(BlockRef(
                "domain.project", project_version,
                {{"name", "TextFoundry"},
                 {"stack", "C++23, ObjectBox, CMake, vcpkg"},
                 {"goals", "composable text templating with versioning"},
                 {"features", "blocks, compositions, render-time params"}}))
            .AddSeparator(SeparatorType::Paragraph)

            // Guidelines
            .AddBlockRef(BlockRef("domain.guidelines", guidelines_version))

            // Constraints
            .AddBlockRef(BlockRef(
                "constraint.cot", cot_version,
                {{"checkpoints", "architecture, security, perf, tests, style"},
                 {"format", "structured JSON"}}))
            .AddBlockRef(BlockRef("constraint.json", json_version))
            .AddBlockRef(BlockRef("constraint.security", security_version,
                                  {{"custom_checks",
                                    "buffer overflows, race conditions, "
                                    "injection in templates"}}))
            .AddBlockRef(BlockRef("constraint.performance", perf_version,
                                  {{"time_complexity", "O(n)"},
                                   {"space_complexity", "O(1)"},
                                   {"bottlenecks", "recursive rendering"}}))
            .AddSeparator(SeparatorType::Paragraph)

            // Static code to review
            .AddStaticText(R"(## Code To Review
```cpp
// Simplified renderer example
std::string render(const Composition& comp, const RenderContext& ctx) {
    std::string result;
    for (const auto& frag : comp.fragments()) {
        if (frag.IsBlockRef()) {
            auto block = LoadBlock(frag.BlockId(), frag.version());
            result += expand_template(block.template(), ResolveParams(block, ctx));
        }
    }
    return result;
}
```
)")

            // Examples
            .AddBlockRef(BlockRef(
                "examples.bad", bad_ex_version,
                {{"code",
                  R"(int* ptr = new int; delete ptr; // leak if exception)"},
                 {"issues", "manual memory, no RAII"}}))
            .AddBlockRef(BlockRef(
                "examples.good", good_ex_version,
                {{"code",
                  R"(std::unique_ptr<int> ptr = std::make_unique<int>(42); )"},
                 {"why_good", "RAII, exception safe"}}))

            .AddSeparator(SeparatorType::Newline)

            // Footer
            .AddBlockRef(BlockRef("meta.footer", footer_version,
                                  {{"block_count", "12"}}));

    auto comp_draft = comp_builder.build();
    auto comp_pub =
        engine.PublishComposition(std::move(comp_draft), Version{1, 0});
    if (!comp_pub.HasValue()) {
      std::cerr << "Comp error: " << comp_pub.error().message << "\n";
      return 1;
    }
    Version comp_version = comp_pub.value().version();
    std::cout << std::format("✅ Composition published v{}.{}\n\n",
                             comp_version.major, comp_version.minor);

    // Render with runtime context
    std::cout << "🎨 Rendering...\n";
    RenderContext ctx;
    ctx.WithParam("custom", "runtime override")
        .WithLanguage("en")
        .with_strict_mode(true);

    auto result =
        engine.Render("advanced.code_review_prompt", comp_version, ctx);
    print_render_result(result);

    // List all
    std::cout << "📋 Published blocks:\n";
    auto all_blocks = engine.ListBlocks();
    for (const auto& bid : all_blocks) {
      auto ver_res = engine.GetLatestBlockVersion(bid);
      if (ver_res.HasValue()) {
        auto ver = ver_res.value();
        std::cout << std::format("  - {} v{}.{}\n", bid, ver.major, ver.minor);
      }
    }

    std::cout << "\n📋 Published compositions:\n";
    auto all_comps = engine.ListCompositions();
    for (const auto& cid : all_comps) {
      auto ver_res = engine.GetLatestCompositionVersion(cid);
      if (ver_res.HasValue()) {
        auto ver = ver_res.value();
        std::cout << std::format("  - {} v{}.{}\n", cid, ver.major, ver.minor);
      }
    }

    std::cout << "\n✅ Advanced example complete! Generated a full LLM prompt "
                 "using 12 blocks.\n";
    return 0;

  } catch (const std::exception& e) {
    std::cerr << "Critical error: " << e.what() << "\n";
    return 1;
  }
}