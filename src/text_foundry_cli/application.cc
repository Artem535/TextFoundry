// Copyright TextFoundry CLI
// Reference client for Text Engine (per PRD MVP)

#include "application.h"
#include <CLI/CLI.hpp>
#include <CLI/App.hpp>
#include <iostream>
#include <sstream>
#include <string_view>
#include "../textfoundry_engine/tf/block.h"
#include "../textfoundry_engine/tf/composition.h"
#include "../textfoundry_engine/tf/error.h"
#include "../textfoundry_engine/tf/renderer.h"

// ============================================================================
// Helper methods implementation
// ============================================================================

tf::Params Application::parseParams(const std::vector<std::string>& kv_list) {
    tf::Params params;
    for (const auto& kv : kv_list) {
        size_t eq_pos = kv.find('=');
        if (eq_pos != std::string::npos) {
            params[kv.substr(0, eq_pos)] = kv.substr(eq_pos + 1);
        } else {
            throw std::invalid_argument("Invalid param format: " + kv + " (expected name=value)");
        }
    }
    return params;
}

tf::Version Application::parseVersion(const std::string& version_str) {
    size_t dot_pos = version_str.find('.');
    if (dot_pos == std::string::npos) {
        throw std::invalid_argument("Invalid version format: " + version_str + " (expected major.minor)");
    }
    uint16_t major = static_cast<uint16_t>(std::stoi(version_str.substr(0, dot_pos)));
    uint16_t minor = static_cast<uint16_t>(std::stoi(version_str.substr(dot_pos + 1)));
    return tf::Version{major, minor};
}

void Application::printError(const std::string& message) {
    std::cerr << "Error: " << message << std::endl;
}

void Application::outputTextList(const std::vector<std::string>& items, std::ostream& out) {
    for (const auto& item : items) {
        out << item << std::endl;
    }
}

// ============================================================================
// Block command handlers
// ============================================================================

void Application::handleBlockCreate(const std::string& block_id, const std::string& block_template,
                                    const std::vector<std::string>& block_defaults_list,
                                    const std::vector<std::string>& block_tags_list,
                                    tf::BlockType block_type, const std::string& block_description,
                                    const std::string& block_lang) {
    init_engine();

    try {
        auto defaults = parseParams(block_defaults_list);

        auto builder = tf::BlockDraftBuilder(block_id)
            .with_template(tf::Template(block_template))
            .with_type(block_type)
            .with_language(block_lang);

        if (!defaults.empty()) {
            builder.with_defaults(std::move(defaults));
        }

        for (const auto& tag : block_tags_list) {
            builder.with_tag(tag);
        }

        if (!block_description.empty()) {
            builder.with_description(block_description);
        }

        auto draft = builder.build();

        if (settings_.dry_run) {
            std::cout << "Dry-run: Would create block " << block_id << std::endl;
            return;
        }

        auto result = engine_.value().publish_block(std::move(draft), tf::Engine::VersionBump::Minor);
        if (result.has_error()) {
            printError(result.error().message);
            throw CLI::RuntimeError(1);
        }

        auto pub = result.value();
        std::cout << "Published block: " << pub.id() << "@" << pub.version().to_string() << std::endl;
    } catch (const std::invalid_argument& e) {
        printError(e.what());
        throw CLI::RuntimeError(1);
    }
}

void Application::handleBlockPublish(const std::string& block_id, const std::string& version_str) {
    init_engine();

    try {
        tf::Version explicit_version = parseVersion(version_str);

        auto load_result = engine_.value().load_block(block_id);
        if (load_result.has_error()) {
            printError("Block not found: " + block_id);
            throw CLI::RuntimeError(1);
        }

        auto block = load_result.value();
        if (block.state() != tf::BlockState::Draft) {
            printError("Block is not in Draft state");
            throw CLI::RuntimeError(1);
        }

        auto draft = tf::BlockDraftBuilder(block_id)
            .with_template(block.templ())
            .with_type(block.type())
            .with_defaults(block.defaults())
            .build();

        if (settings_.dry_run) {
            std::cout << "Dry-run: Would publish " << block_id << "@" << version_str << std::endl;
            return;
        }

        auto result = engine_.value().publish_block(std::move(draft), explicit_version);
        if (result.has_error()) {
            printError(result.error().message);
            throw CLI::RuntimeError(1);
        }

        auto pub = result.value();
        std::cout << "Published block: " << pub.id() << "@" << pub.version().to_string() << std::endl;
    } catch (const std::exception& e) {
        printError(std::string("Failed to publish: ") + e.what());
        throw CLI::RuntimeError(1);
    }
}

void Application::handleBlockList(const std::optional<tf::BlockType>& type_filter) {
    init_engine();

    auto blocks = engine_.value().list_blocks(type_filter);

    if (blocks.empty()) {
        std::cout << "No blocks found" << std::endl;
    } else {
        std::cout << "Blocks (" << blocks.size() << "):" << std::endl;
        outputTextList(blocks, std::cout);
    }
}

void Application::handleBlockDeprecate(const std::string& block_id, const std::string& version_str) {
    init_engine();

    try {
        tf::Version version = parseVersion(version_str);

        if (settings_.dry_run) {
            std::cout << "Dry-run: Would deprecate " << block_id << "@" << version_str << std::endl;
            return;
        }

        auto err = engine_.value().deprecate_block(block_id, version);
        if (err.is_error()) {
            printError(err.message);
            throw CLI::RuntimeError(1);
        }

        std::cout << "Deprecated block: " << block_id << "@" << version_str << std::endl;
    } catch (const std::exception& e) {
        printError(std::string("Failed to deprecate: ") + e.what());
        throw CLI::RuntimeError(1);
    }
}

void Application::handleBlockInspect(const std::string& block_id, const std::optional<std::string>& version_str_opt) {
    init_engine();

    std::optional<tf::Result<tf::Block>> result;

    if (version_str_opt) {
        tf::Version version = parseVersion(*version_str_opt);
        result = engine_.value().load_block(block_id, version);
    } else {
        result = engine_.value().load_block(block_id);
    }

    if (result->has_error()) {
        printError("Block not found: " + block_id);
        throw CLI::RuntimeError(1);
    }

    auto block = result->value();

    std::cout << "Block: " << block.id() << std::endl;
    std::cout << "  Version: " << block.version().to_string() << std::endl;
    std::cout << "  Type: " << static_cast<int>(block.type()) << std::endl;
    std::cout << "  Template: " << block.templ().content() << std::endl;
    std::cout << "  Language: " << block.language() << std::endl;
    if (!block.defaults().empty()) {
        std::cout << "  Defaults:" << std::endl;
        for (const auto& [k, v] : block.defaults()) {
            std::cout << "    " << k << "=" << v << std::endl;
        }
    }
}

// ============================================================================
// Composition command handlers
// ============================================================================

void Application::handleCompCreate(const std::string& comp_id, const std::vector<std::string>& block_refs_list,
                                   const std::vector<std::string>& static_texts, const std::string& description) {
    init_engine();

    try {
        tf::CompositionDraftBuilder builder(comp_id);

        if (!description.empty()) {
            builder.with_description(description);
        }

        for (const auto& ref_str : block_refs_list) {
            size_t at_pos = ref_str.find('@');
            if (at_pos == std::string::npos) {
                throw std::invalid_argument("Block reference must include version: " + ref_str +
                                           " (expected block_id@version)");
            }

            std::string block_id = ref_str.substr(0, at_pos);
            std::string rest = ref_str.substr(at_pos + 1);

            size_t q_pos = rest.find('?');
            std::string version_str = (q_pos != std::string::npos) ? rest.substr(0, q_pos) : rest;
            std::string params_str = (q_pos != std::string::npos) ? rest.substr(q_pos + 1) : "";

            tf::Version version = parseVersion(version_str);

            tf::Params local_params;
            if (!params_str.empty()) {
                size_t start = 0;
                while (start < params_str.size()) {
                    size_t amp_pos = params_str.find('&', start);
                    std::string kv = (amp_pos == std::string::npos)
                        ? params_str.substr(start)
                        : params_str.substr(start, amp_pos - start);

                    size_t eq_pos = kv.find('=');
                    if (eq_pos != std::string::npos) {
                        local_params[kv.substr(0, eq_pos)] = kv.substr(eq_pos + 1);
                    }

                    if (amp_pos == std::string::npos) break;
                    start = amp_pos + 1;
                }
            }

            builder.add_block_ref(block_id, version.major, version.minor, local_params);
        }

        for (const auto& text : static_texts) {
            builder.add_static_text(text);
        }

        auto draft = builder.build();

        if (settings_.dry_run) {
            std::cout << "Dry-run: Would create composition " << comp_id << std::endl;
            return;
        }

        auto result = engine_.value().publish_composition(std::move(draft), tf::Engine::VersionBump::Minor);
        if (result.has_error()) {
            printError(result.error().message);
            throw CLI::RuntimeError(1);
        }

        auto pub = result.value();
        std::cout << "Published composition: " << pub.id() << "@" << pub.version().to_string() << std::endl;
    } catch (const std::exception& e) {
        printError(e.what());
        throw CLI::RuntimeError(1);
    }
}

void Application::handleCompList() {
    init_engine();

    auto comps = engine_.value().list_compositions();

    if (comps.empty()) {
        std::cout << "No compositions found" << std::endl;
    } else {
        std::cout << "Compositions (" << comps.size() << "):" << std::endl;
        outputTextList(comps, std::cout);
    }
}

void Application::handleCompDeprecate(const std::string& comp_id, const std::string& version_str) {
    init_engine();

    try {
        tf::Version version = parseVersion(version_str);

        if (settings_.dry_run) {
            std::cout << "Dry-run: Would deprecate composition " << comp_id << "@" << version_str << std::endl;
            return;
        }

        auto err = engine_.value().deprecate_composition(comp_id, version);
        if (err.is_error()) {
            printError(err.message);
            throw CLI::RuntimeError(1);
        }

        std::cout << "Deprecated composition: " << comp_id << "@" << version_str << std::endl;
    } catch (const std::exception& e) {
        printError(std::string("Failed to deprecate: ") + e.what());
        throw CLI::RuntimeError(1);
    }
}

void Application::handleCompInspect(const std::string& comp_id, const std::optional<std::string>& version_str_opt) {
    init_engine();

    std::optional<tf::Result<tf::Composition>> result;

    if (version_str_opt) {
        tf::Version version = parseVersion(*version_str_opt);
        result = engine_.value().load_composition(comp_id, version);
    } else {
        result = engine_.value().load_composition(comp_id);
    }

    if (result->has_error()) {
        printError("Composition not found: " + comp_id);
        throw CLI::RuntimeError(1);
    }

    auto comp = result->value();

    std::cout << "Composition: " << comp.id() << std::endl;
    std::cout << "  Version: " << comp.version().to_string() << std::endl;
    std::cout << "  Fragments: " << comp.fragmentCount() << std::endl;
}

// ============================================================================
// Render command handler
// ============================================================================

void Application::handleRender(const std::string& comp_id, const std::optional<std::string>& version_str_opt,
                               const std::vector<std::string>& runtime_params_list, bool normalize) {
    init_engine();

    try {
        auto params = parseParams(runtime_params_list);

        tf::RenderContext ctx;
        ctx.params = std::move(params);
        ctx.strictMode = settings_.strict_mode;
        if (settings_.target_language) {
            ctx.targetLanguage = settings_.target_language;
        }

        std::optional<tf::Result<tf::RenderResult>> result;

        if (version_str_opt) {
            tf::Version version = parseVersion(*version_str_opt);
            result = engine_.value().render(comp_id, version, ctx);
        } else {
            result = engine_.value().render(comp_id, ctx);
        }

        if (result->has_error()) {
            printError(result->error().message);
            throw CLI::RuntimeError(1);
        }

        std::string text = result->value().text;

        if (normalize) {
            if (!engine_.value().hasNormalizer()) {
                printError("Normalization requested but no Normalizer configured");
                throw CLI::RuntimeError(1);
            }
        }

        std::cout << text << std::endl;
    } catch (const std::invalid_argument& e) {
        printError(e.what());
        throw CLI::RuntimeError(1);
    } catch (const std::exception& e) {
        printError(std::string("Render failed: ") + e.what());
        throw CLI::RuntimeError(1);
    }
}

// ============================================================================
// Validate command handler
// ============================================================================

void Application::handleValidate(const std::string& entity_id, const std::string& entity_type) {
    init_engine();

    tf::Error err;
    if (entity_type == "block") {
        err = engine_.value().validate_block(entity_id);
    } else {
        err = engine_.value().validate_composition(entity_id);
    }

    if (err.is_error()) {
        std::cerr << "Validation failed: " << err.message << std::endl;
        throw CLI::RuntimeError(1);
    }

    std::cout << "Valid: " << entity_id << std::endl;
}

// ============================================================================
// CLI setup methods
// ============================================================================

void Application::setupGlobalOptions(CLI::App& app) {
    app.option_defaults()->configurable();
    app.add_option("-p,--project", settings_.project_key, "Project key for namespacing")
       ->default_val("default");
    app.add_option("-d,--data", settings_.data_path, "Engine data path (default: memory:objectbox)")
       ->default_val("memory:objectbox");
    app.add_flag("--dry-run", settings_.dry_run, "Dry-run mode (no publish/save)");
    app.add_flag("--strict", settings_.strict_mode, "Strict mode (fail on missing params)");
    app.add_option("-l,--lang", settings_.target_language, "Target language for rendering (e.g., en, ru)");
}

void Application::setupBlockCommands(CLI::App& app) {
    auto block_cmd = app.add_subcommand("block", "Block operations (create, publish, list, deprecate, inspect)");
    block_cmd->require_subcommand(1);

    // ---- block create ----
    {
        std::string block_id, block_template, block_description;
        std::vector<std::string> block_defaults_list;
        std::vector<std::string> block_tags_list;
        tf::BlockType block_type = tf::BlockType::Domain;
        std::string block_lang = "en";

        auto cmd = block_cmd->add_subcommand("create", "Create and publish a new block");
        cmd->add_option("id", block_id, "Block ID (e.g., greeting.basic, role.analyst)")
           ->required();
        cmd->add_option("-t,--template", block_template, "Template string with {{placeholders}}")
           ->required();
        cmd->add_option("--default", block_defaults_list, "Default param (repeatable): name=value")
           ->take_all();
        cmd->add_option("--type", block_type, "Block type: role|constraint|style|domain|meta")
           ->default_val(tf::BlockType::Domain);
        cmd->add_option("--tag", block_tags_list, "Tag (repeatable)")
           ->take_all();
        cmd->add_option("--desc", block_description, "Block description");
        cmd->add_option("--lang", block_lang, "Block language (default: en)")
           ->default_val("en");

        cmd->callback([this, &block_id, &block_template, &block_defaults_list,
                       &block_tags_list, &block_type, &block_description, &block_lang]() {
            handleBlockCreate(block_id, block_template, block_defaults_list, block_tags_list,
                              block_type, block_description, block_lang);
        });
    }

    // ---- block publish ----
    {
        std::string block_id;
        std::string version_str;

        auto cmd = block_cmd->add_subcommand("publish", "Publish a block with explicit version");
        cmd->add_option("id", block_id, "Block ID")
           ->required();
        cmd->add_option("version", version_str, "Version (e.g., 1.0, 2.5)")
           ->required();

        cmd->callback([this, &block_id, &version_str]() {
            handleBlockPublish(block_id, version_str);
        });
    }

    // ---- block list ----
    {
        std::optional<tf::BlockType> type_filter;
        bool show_deprecated = false;

        auto cmd = block_cmd->add_subcommand("list", "List all blocks");
        cmd->add_option("--type", type_filter, "Filter by type: role|constraint|style|domain|meta");
        cmd->add_flag("--deprecated", show_deprecated, "Include deprecated blocks");

        cmd->callback([this, &type_filter]() {
            handleBlockList(type_filter);
        });
    }

    // ---- block deprecate ----
    {
        std::string block_id;
        std::string version_str;

        auto cmd = block_cmd->add_subcommand("deprecate", "Mark a block version as deprecated");
        cmd->add_option("id", block_id, "Block ID")
           ->required();
        cmd->add_option("version", version_str, "Version to deprecate (e.g., 1.0)")
           ->required();

        cmd->callback([this, &block_id, &version_str]() {
            handleBlockDeprecate(block_id, version_str);
        });
    }

    // ---- block inspect ----
    {
        std::string block_id;
        std::optional<std::string> version_str_opt;

        auto cmd = block_cmd->add_subcommand("inspect", "Show block details");
        cmd->add_option("id", block_id, "Block ID")
           ->required();
        cmd->add_option("-v,--version", version_str_opt, "Specific version (default: latest)");

        cmd->callback([this, &block_id, &version_str_opt]() {
            handleBlockInspect(block_id, version_str_opt);
        });
    }
}

void Application::setupCompCommands(CLI::App& app) {
    auto comp_cmd = app.add_subcommand("comp", "Composition operations (create, list, deprecate, inspect)");
    comp_cmd->require_subcommand(1);

    // ---- comp create ----
    {
        std::string comp_id;
        std::vector<std::string> block_refs_list;
        std::vector<std::string> static_texts;
        std::string description;

        auto cmd = comp_cmd->add_subcommand("create", "Create and publish a composition from blocks");
        cmd->add_option("id", comp_id, "Composition ID (e.g., welcome.message)")
           ->required();
        cmd->add_option("-b,--block", block_refs_list, "Block reference: id@version or id@version?param=value")
           ->take_all();
        cmd->add_option("-t,--text", static_texts, "Static text fragment (repeatable)")
           ->take_all();
        cmd->add_option("--desc", description, "Composition description");

        cmd->callback([this, &comp_id, &block_refs_list, &static_texts, &description]() {
            handleCompCreate(comp_id, block_refs_list, static_texts, description);
        });
    }

    // ---- comp list ----
    {
        auto cmd = comp_cmd->add_subcommand("list", "List all compositions");

        cmd->callback([this]() {
            handleCompList();
        });
    }

    // ---- comp deprecate ----
    {
        std::string comp_id;
        std::string version_str;

        auto cmd = comp_cmd->add_subcommand("deprecate", "Mark a composition version as deprecated");
        cmd->add_option("id", comp_id, "Composition ID")
           ->required();
        cmd->add_option("version", version_str, "Version to deprecate (e.g., 1.0)")
           ->required();

        cmd->callback([this, &comp_id, &version_str]() {
            handleCompDeprecate(comp_id, version_str);
        });
    }

    // ---- comp inspect ----
    {
        std::string comp_id;
        std::optional<std::string> version_str_opt;

        auto cmd = comp_cmd->add_subcommand("inspect", "Show composition details");
        cmd->add_option("id", comp_id, "Composition ID")
           ->required();
        cmd->add_option("-v,--version", version_str_opt, "Specific version (default: latest)");

        cmd->callback([this, &comp_id, &version_str_opt]() {
            handleCompInspect(comp_id, version_str_opt);
        });
    }
}

void Application::setupRenderCommand(CLI::App& app) {
    std::string comp_id;
    std::optional<std::string> version_str_opt;
    std::vector<std::string> runtime_params_list;
    bool normalize = false;

    auto cmd = app.add_subcommand("render", "Render a composition to text");
    cmd->add_option("id", comp_id, "Composition ID")
       ->required();
    cmd->add_option("-v,--version", version_str_opt, "Specific version (default: latest)");
    cmd->add_option("-p,--param", runtime_params_list, "Runtime param: name=value (repeatable)")
       ->take_all();
    cmd->add_flag("--normalize", normalize, "Apply semantic normalization (requires Normalizer)");

    cmd->callback([this, &comp_id, &version_str_opt, &runtime_params_list, &normalize]() {
        handleRender(comp_id, version_str_opt, runtime_params_list, normalize);
    });
}

void Application::setupValidateCommand(CLI::App& app) {
    std::string entity_id;
    std::string entity_type;

    auto cmd = app.add_subcommand("validate", "Validate a block or composition");
    cmd->add_option("type", entity_type, "Type: block|comp")
       ->required()
       ->check(CLI::IsMember({"block", "comp"}));
    cmd->add_option("id", entity_id, "Entity ID")
       ->required();

    cmd->callback([this, &entity_id, &entity_type]() {
        handleValidate(entity_id, entity_type);
    });
}

// ============================================================================
// Engine initialization
// ============================================================================

void Application::init_engine() {
    if (engine_.has_value()) return;

    tf::EngineConfig config{
        .project_key = settings_.project_key,
        .strict_mode = settings_.strict_mode,
        .default_data_path = settings_.data_path
    };

    engine_.emplace(config);
    engine_->full_init();
}

// ============================================================================
// Main run method
// ============================================================================

int Application::run(int argc, char **argv) {
    CLI::App app{"tf - TextFoundry CLI: Reference client for Text Engine (per PRD MVP)",
                 "tf"};
    app.set_version_flag("-v,--version", "0.1.0 (MVP)");

    setupGlobalOptions(app);

    app.require_subcommand(1);

    setupBlockCommands(app);
    setupCompCommands(app);
    setupRenderCommand(app);
    setupValidateCommand(app);

    CLI11_PARSE(app, argc, argv);
    return 0;
}
