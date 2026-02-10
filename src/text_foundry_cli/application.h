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
  int Run(int argc, char** argv);

 private:
  ApplicationSettings settings_;
  std::optional<tf::Engine> engine_;
  std::optional<tf::Tui> tui_;

  void InitEngine();

  // CLI setup methods
  void SetupGlobalOptions(CLI::App& app);
  void SetupBlockCommands(CLI::App& app);
  void SetupCompCommands(CLI::App& app);
  void SetupRenderCommand(CLI::App& app);
  void SetupValidateCommand(CLI::App& app);
  void SetupTuiCommands(CLI::App& app);

  // Block command handlers
  void HandleBlockCreate(const std::string& BlockId,
                         const std::string& block_template,
                         const std::vector<std::string>& block_defaults_list,
                         const std::vector<std::string>& block_tags_list,
                         tf::BlockType block_type,
                         const std::string& block_description,
                         const std::string& block_lang);
  void HandleBlockPublish(const std::string& BlockId,
                          const std::string& version_str);
  void HandleBlockList(const std::optional<tf::BlockType>& type_filter);
  void HandleBlockDeprecate(const std::string& BlockId,
                            const std::string& version_str);
  void HandleBlockInspect(const std::string& BlockId,
                          const std::optional<std::string>& version_str_opt);

  // Composition command handlers
  void HandleCompCreate(const std::string& comp_id,
                        const std::vector<std::string>& block_refs_list,
                        const std::vector<std::string>& static_texts,
                        const std::string& description);
  void HandleCompList();
  void HandleCompDeprecate(const std::string& comp_id,
                           const std::string& version_str);
  void HandleCompInspect(const std::string& comp_id,
                         const std::optional<std::string>& version_str_opt);

  // Render command handler
  void HandleRender(const std::string& comp_id,
                    const std::optional<std::string>& version_str_opt,
                    const std::vector<std::string>& runtime_params_list,
                    bool normalize);

  // Validate command handler
  void HandleValidate(const std::string& entity_id,
                      const std::string& entity_type);

  // Helper methods
  static tf::Params ParseParams(const std::vector<std::string>& kv_list);
  static tf::Version ParseVersion(const std::string& version_str);
  static void PrintError(const std::string& message);
  static void OutputTextList(const std::vector<std::string>& items,
                             std::ostream& out);
};
