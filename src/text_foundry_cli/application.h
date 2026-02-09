//
// Created by a.durynin on 02.02.2026.
//

#pragma once

#include <CLI/CLI.hpp>
#include <ftxui/dom/canvas.hpp>
#include <optional>
#include <string>
#include <vector>

#include "applicationsettings.h"
#include "tf/engine.h"
#include "tf/types.h"
#include "tui.h"

class Application {
 public:
  Application();
  int run(int argc, char** argv);

 private:
  ApplicationSettings settings_;
  std::optional<tf::Engine> engine_;
  std::optional<tf::Tui> tui_;

  void init_engine();

  // CLI setup methods
  void setup_global_options(CLI::App& app);
  void setup_block_commands(CLI::App& app);
  void setup_comp_commands(CLI::App& app);
  void setup_render_command(CLI::App& app);
  void setup_validate_command(CLI::App& app);
  void setup_tui_commands(CLI::App& app);

  // Block command handlers
  void handle_block_create(const std::string& block_id,
                           const std::string& block_template,
                           const std::vector<std::string>& block_defaults_list,
                           const std::vector<std::string>& block_tags_list,
                           tf::BlockType block_type,
                           const std::string& block_description,
                           const std::string& block_lang);
  void handle_block_publish(const std::string& block_id,
                            const std::string& version_str);
  void handle_block_list(const std::optional<tf::BlockType>& type_filter);
  void handle_block_deprecate(const std::string& block_id,
                              const std::string& version_str);
  void handle_block_inspect(const std::string& block_id,
                            const std::optional<std::string>& version_str_opt);

  // Composition command handlers
  void handle_comp_create(const std::string& comp_id,
                          const std::vector<std::string>& block_refs_list,
                          const std::vector<std::string>& static_texts,
                          const std::string& description);
  void handle_comp_list();
  void handle_comp_deprecate(const std::string& comp_id,
                             const std::string& version_str);
  void handle_comp_inspect(const std::string& comp_id,
                           const std::optional<std::string>& version_str_opt);

  // Render command handler
  void handle_render(const std::string& comp_id,
                     const std::optional<std::string>& version_str_opt,
                     const std::vector<std::string>& runtime_params_list,
                     bool normalize);

  // Validate command handler
  void handle_validate(const std::string& entity_id,
                       const std::string& entity_type);

  // Helper methods
  static tf::Params parse_params(const std::vector<std::string>& kv_list);
  static tf::Version parse_version(const std::string& version_str);
  static void print_error(const std::string& message);
  static void output_text_list(const std::vector<std::string>& items,
                               std::ostream& out);
};
