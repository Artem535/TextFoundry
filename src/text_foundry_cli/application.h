//
// Created by a.durynin on 02.02.2026.
//

#pragma once

#include "applicationsettings.h"
#include <CLI/CLI.hpp>
#include <optional>
#include <string>
#include <vector>
#include "../textfoundry_engine/tf/engine.h"
#include "../textfoundry_engine/tf/types.h"

class Application {
public:
  int run(int argc, char **argv);

private:
  void init_engine();

  // CLI setup methods
  void setupGlobalOptions(CLI::App& app);
  void setupBlockCommands(CLI::App& app);
  void setupCompCommands(CLI::App& app);
  void setupRenderCommand(CLI::App& app);
  void setupValidateCommand(CLI::App& app);

  // Block command handlers
  void handleBlockCreate(const std::string& block_id, const std::string& block_template,
                         const std::vector<std::string>& block_defaults_list,
                         const std::vector<std::string>& block_tags_list,
                         tf::BlockType block_type, const std::string& block_description,
                         const std::string& block_lang);
  void handleBlockPublish(const std::string& block_id, const std::string& version_str);
  void handleBlockList(const std::optional<tf::BlockType>& type_filter);
  void handleBlockDeprecate(const std::string& block_id, const std::string& version_str);
  void handleBlockInspect(const std::string& block_id, const std::optional<std::string>& version_str_opt);

  // Composition command handlers
  void handleCompCreate(const std::string& comp_id, const std::vector<std::string>& block_refs_list,
                        const std::vector<std::string>& static_texts, const std::string& description);
  void handleCompList();
  void handleCompDeprecate(const std::string& comp_id, const std::string& version_str);
  void handleCompInspect(const std::string& comp_id, const std::optional<std::string>& version_str_opt);

  // Render command handler
  void handleRender(const std::string& comp_id, const std::optional<std::string>& version_str_opt,
                    const std::vector<std::string>& runtime_params_list, bool normalize);

  // Validate command handler
  void handleValidate(const std::string& entity_id, const std::string& entity_type);

  // Helper methods
  tf::Params parseParams(const std::vector<std::string>& kv_list);
  tf::Version parseVersion(const std::string& version_str);
  void printError(const std::string& message);
  void outputTextList(const std::vector<std::string>& items, std::ostream& out);

  ApplicationSettings settings_;
  std::optional<tf::Engine> engine_;
};
