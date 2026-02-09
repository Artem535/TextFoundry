#include <tf/engine.h>

#include <format>
#include <iostream>
#include <vector>

using namespace tf;

void print_render_result(const Result<RenderResult>& result) {
  if (result.has_error()) {
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
    EngineConfig config{.project_key = "demo",
                        .strict_mode = true,
                        .default_data_path = "memory:test_app"};
    Engine engine(config);

    std::cout
        << "=== Advanced TextFoundry Example: LLM Code Review Prompt ===\n\n";

    // Publish 12+ blocks of different types
    std::cout << "📦 Publishing blocks...\n";

    // 1. Meta header
    auto header_draft = BlockDraftBuilder("meta.header")
                            .with_type(BlockType::Meta)
                            .with_template(Template(R"(# {{title}}
**Author:** {{author}} | **Date:** {{date}} | **Audience:** {{audience}})"))
                            .with_default("author", "Engineering Team")
                            .with_default("date", "2026-02-02")
                            .with_default("audience", "developers")
                            .with_description("Document header with metadata")
                            .build();
    auto header_pub =
        engine.publish_block(std::move(header_draft), Version{1, 0});
    if (!header_pub.has_value()) {
      std::cerr << "Header error: " << header_pub.error().message << "\n";
      return 1;
    }
    Version header_version = header_pub.value().version();

    // 2. Role system
    auto system_draft =
        BlockDraftBuilder("role.system")
            .with_type(BlockType::Role)
            .with_template(Template(
                R"(You are a {{role_type}} assistant specialized in {{domain}}. 
Follow all instructions precisely. )"))
            .with_default("role_type", "highly competent")
            .with_default("domain", "software engineering")
            .with_description("System role definition")
            .build();
    auto system_pub =
        engine.publish_block(std::move(system_draft), Version{1, 0});
    Version system_version = system_pub.value().version();

    // 3. Constraint chain-of-thought
    auto cot_draft =
        BlockDraftBuilder("constraint.cot")
            .with_type(BlockType::Constraint)
            .with_template(Template(R"(> **Think step-by-step:**
> 1. Analyze structure
> 2. Check {{checkpoints}}
> 3. Suggest improvements
> 4. Output in {{format}})"))
            .with_default("checkpoints", "security, performance, readability")
            .with_default("format", "JSON")
            .with_description("Chain of thought instruction")
            .build();
    auto cot_pub = engine.publish_block(std::move(cot_draft), Version{1, 0});
    Version cot_version = cot_pub.value().version();

    // 4. Constraint JSON output
    auto json_draft =
        BlockDraftBuilder("constraint.json")
            .with_type(BlockType::Constraint)
            .with_template(Template(
                R"(**Output format:** Valid JSON only matching this schema:
```json
{
  "issues": [{{schema}}],
  "score": {{score_range}},
  "recommendations": "{{rec}}"
}
```)"))
            .with_default("schema", "array of issue objects")
            .with_default("score_range", "0-10")
            .with_default("rec", "actionable steps")
            .with_description("JSON output constraint")
            .build();
    auto json_pub = engine.publish_block(std::move(json_draft), Version{1, 0});
    Version json_version = json_pub.value().version();

    // 5. Domain project context
    auto project_draft = BlockDraftBuilder("domain.project")
                             .with_type(BlockType::Domain)
                             .with_template(Template(R"(## Project Context
**Name:** {{name}}
**Stack:** {{stack}}
**Goals:** {{goals}}
**Key features:** {{features}})"))
                             .with_default("name", "Unnamed Project")
                             .with_default("stack", "C++")
                             .with_default("goals", "high performance")
                             .with_default("features", "text templating")
                             .with_description("Project overview")
                             .build();
    auto project_pub =
        engine.publish_block(std::move(project_draft), Version{1, 0});
    Version project_version = project_pub.value().version();

    // 6. Role code reviewer
    auto reviewer_draft =
        BlockDraftBuilder("role.reviewer")
            .with_type(BlockType::Role)
            .with_template(Template(
                R"(Act as a senior {{level}} code reviewer with 15+ years experience.
Review for: {{criteria}}. 
Be constructive and precise.)"))
            .with_default("level", "software engineer")
            .with_default("criteria", "correctness, style, security")
            .with_description("Code reviewer persona")
            .build();
    auto reviewer_pub =
        engine.publish_block(std::move(reviewer_draft), Version{1, 0});
    Version reviewer_version = reviewer_pub.value().version();

    // 7. Constraint security
    auto security_draft = BlockDraftBuilder("constraint.security")
                              .with_type(BlockType::Constraint)
                              .with_template(Template(R"(> ⚠️ **Security Check:**
> - SQL/NoSQL injection
> - XSS/CSRF
> - Auth bypass
> - {{custom_checks}}
> Flag ALL potential issues!)"))
                              .with_default("custom_checks", "data validation")
                              .with_description("Security constraints")
                              .build();
    auto security_pub =
        engine.publish_block(std::move(security_draft), Version{1, 0});
    Version security_version = security_pub.value().version();

    // 8. Domain coding guidelines
    auto guidelines_draft =
        BlockDraftBuilder("domain.guidelines")
            .with_type(BlockType::Domain)
            .with_template(Template(R"(## Coding Guidelines
- Use {{style_guide}}
- Complexity < {{max_complexity}}
- Tests coverage > {{min_coverage}}%
- {{additional_rules}})"))
            .with_default("style_guide", "Google C++ Style")
            .with_default("max_complexity", "10")
            .with_default("min_coverage", "80")
            .with_default("additional_rules", "No raw pointers")
            .with_description("Coding standards")
            .build();
    auto guidelines_pub =
        engine.publish_block(std::move(guidelines_draft), Version{1, 0});
    Version guidelines_version = guidelines_pub.value().version();

    // 9. Examples bad code
    auto bad_ex_draft =
        BlockDraftBuilder("examples.bad")
            .with_type(BlockType::Domain)
            .with_template(Template(R"(## Bad Example
```cpp
{{code}}
```
**Issues:** {{issues}})"))
            .with_default("issues", "memory leak, no validation")
            .with_description("Bad code example")
            .build();
    auto bad_ex_pub =
        engine.publish_block(std::move(bad_ex_draft), Version{1, 0});
    Version bad_ex_version = bad_ex_pub.value().version();

    // 10. Examples good code
    auto good_ex_draft =
        BlockDraftBuilder("examples.good")
            .with_type(BlockType::Domain)
            .with_template(Template(R"(## Good Example
```cpp
{{code}}
```
**Why good:** {{why_good}})"))
            .with_default("why_good", "RAII, validation, tests")
            .with_description("Good code example")
            .build();
    auto good_ex_pub =
        engine.publish_block(std::move(good_ex_draft), Version{1, 0});
    Version good_ex_version = good_ex_pub.value().version();

    // 11. Constraint performance
    auto perf_draft = BlockDraftBuilder("constraint.performance")
                          .with_type(BlockType::Constraint)
                          .with_template(Template(R"(> **Performance Review:**
> - Time: O({{time_complexity}})
> - Space: O({{space_complexity}})
> - Bottlenecks: {{bottlenecks}})"))
                          .with_default("time_complexity", "n log n")
                          .with_default("space_complexity", "n")
                          .with_default("bottlenecks", "none")
                          .with_description("Performance constraints")
                          .build();
    auto perf_pub = engine.publish_block(std::move(perf_draft), Version{1, 0});
    Version perf_version = perf_pub.value().version();

    // 12. Meta footer
    auto footer_draft = BlockDraftBuilder("meta.footer")
                            .with_type(BlockType::Meta)
                            .with_template(Template(R"(---
**Generated:** {{date}} | **Version:** {{version}}
**Blocks used:** {{block_count}})"))
                            .with_default("date", "2026-02-02")
                            .with_default("version", "1.0")
                            .with_default("block_count", "12")
                            .with_description("Document footer")
                            .build();
    auto footer_pub =
        engine.publish_block(std::move(footer_draft), Version{1, 0});
    if (!footer_pub.has_value()) {
      std::cerr << "Footer error: " << footer_pub.error().message << "\n";
      return 1;
    }
    Version footer_version = footer_pub.value().version();

    std::cout << "✅ All 12 blocks published\n\n";

    // Build massive composition using ALL blocks + static text + separators
    std::cout << "📝 Building advanced composition...\n";

    StructuralStyle structural{
        .blockWrapper = "BLOCK[{{block_id}}]:\\n{{content}}\\n\\n",
        .preamble =
            "Full LLM Code Review Prompt (TextFoundry Advanced Demo)\n\n",
        .delimiter = "\\n---\\n"};
    StyleProfile style{.structural = structural};

    auto comp_builder =
        CompositionDraftBuilder("advanced.code_review_prompt")
            .with_project_key("demo")
            .with_description(
                "Advanced LLM code review prompt using 12 reusable blocks")
            .with_style_profile(style)

            // Header
            .add_block_ref(BlockRef(
                "meta.header", header_version,
                {{"title", "Advanced Code Review for TextFoundry Engine"},
                 {"audience", "senior developers"}}))
            .add_separator(SeparatorType::Paragraph)

            // Static intro
            .add_static_text(R"(## Task
Review the provided C++ code for a templating engine.)")

            // System role
            .add_block_ref(BlockRef("role.system", system_version,
                                    {{"role_type", "world-class"},
                                     {"domain", "C++ software architecture"}}))
            .add_separator(SeparatorType::Newline)

            // Reviewer role
            .add_block_ref(BlockRef(
                "role.reviewer", reviewer_version,
                {{"level", "staff engineer"},
                 {"criteria",
                  "architecture, security, performance, maintainability"}}))

            // Project context
            .add_block_ref(BlockRef(
                "domain.project", project_version,
                {{"name", "TextFoundry"},
                 {"stack", "C++23, ObjectBox, CMake, vcpkg"},
                 {"goals", "composable text templating with versioning"},
                 {"features", "blocks, compositions, render-time params"}}))
            .add_separator(SeparatorType::Paragraph)

            // Guidelines
            .add_block_ref(BlockRef("domain.guidelines", guidelines_version))

            // Constraints
            .add_block_ref(BlockRef(
                "constraint.cot", cot_version,
                {{"checkpoints", "architecture, security, perf, tests, style"},
                 {"format", "structured JSON"}}))
            .add_block_ref(BlockRef("constraint.json", json_version))
            .add_block_ref(BlockRef("constraint.security", security_version,
                                    {{"custom_checks",
                                      "buffer overflows, race conditions, "
                                      "injection in templates"}}))
            .add_block_ref(BlockRef("constraint.performance", perf_version,
                                    {{"time_complexity", "O(n)"},
                                     {"space_complexity", "O(1)"},
                                     {"bottlenecks", "recursive rendering"}}))
            .add_separator(SeparatorType::Paragraph)

            // Static code to review
            .add_static_text(R"(## Code To Review
```cpp
// Simplified renderer example
std::string render(const Composition& comp, const RenderContext& ctx) {
    std::string result;
    for (const auto& frag : comp.fragments()) {
        if (frag.is_block_ref()) {
            auto block = load_block(frag.block_id(), frag.version());
            result += expand_template(block.template(), resolve_params(block, ctx));
        }
    }
    return result;
}
```
)")

            // Examples
            .add_block_ref(BlockRef(
                "examples.bad", bad_ex_version,
                {{"code",
                  R"(int* ptr = new int; delete ptr; // leak if exception)"},
                 {"issues", "manual memory, no RAII"}}))
            .add_block_ref(BlockRef(
                "examples.good", good_ex_version,
                {{"code",
                  R"(std::unique_ptr<int> ptr = std::make_unique<int>(42); )"},
                 {"why_good", "RAII, exception safe"}}))

            .add_separator(SeparatorType::Newline)

            // Footer
            .add_block_ref(BlockRef("meta.footer", footer_version,
                                    {{"block_count", "12"}}));

    auto comp_draft = comp_builder.build();
    auto comp_pub =
        engine.publish_composition(std::move(comp_draft), Version{1, 0});
    if (!comp_pub.has_value()) {
      std::cerr << "Comp error: " << comp_pub.error().message << "\n";
      return 1;
    }
    Version comp_version = comp_pub.value().version();
    std::cout << std::format("✅ Composition published v{}.{}\n\n",
                             comp_version.major, comp_version.minor);

    // Render with runtime context
    std::cout << "🎨 Rendering...\n";
    RenderContext ctx;
    ctx.with_param("custom", "runtime override")
        .with_language("en")
        .with_strict_mode(true);

    auto result =
        engine.render("advanced.code_review_prompt", comp_version, ctx);
    print_render_result(result);

    // List all
    std::cout << "📋 Published blocks:\n";
    auto all_blocks = engine.list_blocks();
    for (const auto& bid : all_blocks) {
      auto ver_res = engine.get_latest_block_version(bid);
      if (ver_res.has_value()) {
        auto ver = ver_res.value();
        std::cout << std::format("  - {} v{}.{}\n", bid, ver.major, ver.minor);
      }
    }

    std::cout << "\n📋 Published compositions:\n";
    auto all_comps = engine.list_compositions();
    for (const auto& cid : all_comps) {
      auto ver_res = engine.get_latest_composition_version(cid);
      if (ver_res.has_value()) {
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