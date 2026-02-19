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
 * @brief Terminal UI for TextFoundry built with FTXUI.
 *
 * The class owns UI state and composes the application from tab components.
 * Blocks tab supports list/details view and modal create/edit workflow.
 */
class Tui {
 public:
  /**
   * @brief Construct TUI bound to a TextFoundry engine instance.
   * @param engine Engine used for block/composition operations.
   */
  explicit Tui(Engine& engine);

  /**
   * @brief Start interactive UI loop.
   *
   * Performs terminal capability checks, creates the root component tree,
   * and enters FTXUI event loop.
   */
  void Run();

 private:
  Engine& engine_;  ///< Backend engine used by tabs and modal workflows.

  // ===== Global UI state =====
  int selected_tab_ = 0;      ///< Active tab index in top-level toggle.
  bool show_welcome_ = true;  ///< Whether startup welcome modal is visible.
  /// Tab names bound to top-level tab toggle.
  std::vector<std::string> tab_names_ = {"Blocks", "Compositions", "Render",
                                         "Settings"};

  // ===== Blocks tab state =====
  std::vector<std::string> block_ids_;  ///< IDs shown in blocks list.
  int selected_block_ = 0;              ///< Selected block index in list.
  std::string details_text_ =
      "Select a block to see details";     ///< Right panel.
  BlockCreationController block_creator_;  ///< Create/edit form state/logic.
  bool show_create_block_modal_ =
      false;  ///< Create/edit block modal visibility.

  // ===== Compositions tab state =====
  std::vector<std::string> comp_ids_;  ///< IDs shown in compositions list.
  int selected_comp_ = 0;              ///< Selected composition index.
  std::string comp_details_text_ =
      "Select a composition to see details";  ///< Right panel.

  // ===== Render tab state =====
  std::string render_comp_id_;  ///< Composition ID input for render tab.
  std::string render_version_;  ///< Optional composition version (major.minor).
  std::string render_params_;   ///< Raw runtime params input.
  std::string render_output_ =
      "Enter composition ID and click Render";  ///< Output.

  // ===== Settings tab state =====
  std::string settings_project_ = "default";  ///< Active project key.
  std::string settings_path_ =
      (fs::path(sago::getConfigHome()) / "TextFoundry")
          .string();              ///< Storage path from platform config dir.
  bool settings_strict_ = false;  ///< Strict render/validation mode toggle.
  std::string settings_status_text_ =
      "Settings are local to TUI session.";  ///< Settings tab status line.

  // ===== Component factories =====
  /**
   * @brief Build root layout with tabs and global event handling.
   * @param screen Interactive screen used for loop control.
   * @return Root FTXUI component.
   */
  ftxui::Component MainLayout(auto& screen);

  /**
   * @brief Build Blocks tab component.
   * @return Blocks list/details pane with modal create/edit actions.
   */
  ftxui::Component BlocksTab();
  /**
   * @brief Build Compositions tab component.
   * @return Compositions tab component.
   */
  ftxui::Component CompositionsTab();
  /**
   * @brief Build Render tab component.
   * @return Render tab component.
   */
  ftxui::Component RenderTab();
  /**
   * @brief Build Settings tab component.
   * @return Settings tab component.
   */
  ftxui::Component SettingsTab();

  // ===== Blocks helpers =====
  /**
   * @brief Reload block IDs from engine and clamp selected index.
   */
  void RefreshBlocksList();
  /**
   * @brief Refresh details text for currently selected block.
   */
  void UpdateSelectedBlockDetails();
  /**
   * @brief Select block by ID in current list if present.
   * @param block_id Block ID to select.
   */
  void SelectBlockById(const std::string& block_id);
  /**
   * @brief Apply common post-publish UI state update.
   * @param block_id Published block ID.
   */
  void OnBlockPublished(const std::string& block_id);

  // ===== Compositions helpers =====
  /**
   * @brief Reload composition IDs from engine and clamp selected index.
   */
  void RefreshCompositionsList();
  /**
   * @brief Refresh details text for currently selected composition.
   */
  void UpdateSelectedCompositionDetails();

};
}  // namespace tf
