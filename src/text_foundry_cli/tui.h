//
// Simplified FTXUI-based TUI for TextFoundry
//

#pragma once

#include <sago/platform_folders.h>

#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <memory>
#include <string>
#include <vector>

#include "block_creation_controller.h"

// Forward declaration
namespace tf {
class Engine;
}

namespace tf {
namespace fs = std::filesystem;

/**
 * Simplified TUI using FTXUI
 *
 * Pattern: Each tab is a Component factory function
 * No complex inheritance, just functions returning Component
 */
class Tui {
 public:
  explicit Tui(Engine& engine);

  // Main entry point
  void run();

 private:
  Engine& engine_;

  // State
  int selected_tab_ = 0;
  bool show_welcome_ = true;
  // Tab state (for data binding)
  std::vector<std::string> tab_names_ = {"Blocks", "Compositions", "Render",
                                         "Settings"};

  // ===== Blocks tab state =====
  std::vector<std::string> block_ids_;
  int selected_block_ = 0;
  std::string details_text_ = "Select a block to see details";
  BlockCreationController block_creator_;
  bool show_create_block_modal_ = false;

  // ===== Compositions tab state =====
  std::vector<std::string> comp_ids_;
  int selected_comp_ = 0;

  // ===== Render tab state =====
  std::string render_comp_id_;
  std::string render_params_;
  std::string render_output_ = "Enter composition ID and click Render";

  // ===== Settings tab state =====
  std::string settings_project_ = "default";
  std::string settings_path_ =
      (fs::path(sago::getConfigHome()) / "TextFoundry").string();
  bool settings_strict_ = false;

  // ===== Component factories =====
  // Each returns a Component (interactive UI element)

  // Main layout with tabs
  ftxui::Component MainLayout(auto& screen);

  // Individual tabs
  ftxui::Component BlocksTab();        // List + details
  ftxui::Component CompositionsTab();  // List of compositions
  ftxui::Component RenderTab();        // Input fields + output
  ftxui::Component SettingsTab();      // Config options

  // ===== Blocks helpers =====
  void RefreshBlocksList();
  void UpdateSelectedBlockDetails();
  void SelectBlockById(const std::string& block_id);
};
}  // namespace tf
