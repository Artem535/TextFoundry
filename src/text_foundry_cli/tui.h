//
// Simplified FTXUI-based TUI for TextFoundry
//

#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>
#include <vector>
#include <memory>

// Forward declaration
namespace tf {
  class Engine;
}

namespace tf {
  /**
   * Simplified TUI using FTXUI
   *
   * Pattern: Each tab is a Component factory function
   * No complex inheritance, just functions returning Component
   */
  class Tui {
  public:
    explicit Tui(Engine &engine);

    // Main entry point
    void run();

  private:
    Engine &engine_;

    // State
    int selected_tab_ = 0;
    bool show_welcome_ = true;
    // Tab state (for data binding)
    std::vector<std::string> tab_names_ = {"Blocks", "Compositions", "Render", "Settings"};

    // ===== Blocks tab state =====
    std::vector<std::string> block_ids_;
    int selected_block_ = 0;
    std::string details_text_ = "Select a block to see details";

    // ===== Compositions tab state =====
    std::vector<std::string> comp_ids_;
    int selected_comp_ = 0;

    // ===== Render tab state =====
    std::string render_comp_id_;
    std::string render_params_;
    std::string render_output_ = "Enter composition ID and click Render";

    // ===== Settings tab state =====
    std::string settings_project_ = "default";
    std::string settings_path_ = "./tf_data";
    bool settings_strict_ = false;

    // ===== Component factories =====
    // Each returns a Component (interactive UI element)

    // Main layout with tabs
    ftxui::Component main_layout(auto &screen);


    // Individual tabs
    ftxui::Component blocks_tab(); // List + details
    ftxui::Component compositions_tab(); // List of compositions
    ftxui::Component render_tab(); // Input fields + output
    ftxui::Component settings_tab(); // Config options
  };
} // namespace tf
