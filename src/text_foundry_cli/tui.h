//
// Simplified FTXUI-based TUI for TextFoundry
//

#pragma once

#include <sago/platform_folders.h>

#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <memory>
#include <optional>
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
  enum class CompositionFragmentEditMode {
    Append,
    InsertBefore,
    InsertAfter,
    Replace,
  };

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
  std::vector<std::string> block_ids_;  ///< All known block IDs.
  std::vector<std::string> block_folders_;  ///< Virtual folders from BlockId namespaces.
  std::vector<std::string> visible_block_ids_;  ///< IDs shown in filtered blocks list.
  int selected_block_folder_ = 0;       ///< Selected virtual folder index.
  int selected_block_ = 0;              ///< Selected block index in filtered list.
  std::string details_text_ =
      "Select a block to see details";     ///< Right panel.
  std::optional<std::string> detail_id_;   ///< Selected block ID.
  std::optional<std::string> detail_type_;  ///< Selected block type.
  std::optional<std::string> detail_version_;  ///< Selected block version.
  std::optional<std::string> detail_template_;  ///< Selected block template.
  float detail_template_scroll_y_ =
      0.f;  ///< Block template scroll position [0..1].
  BlockCreationController block_creator_;  ///< Create/edit form state/logic.
  bool show_create_block_modal_ =
      false;  ///< Create/edit block modal visibility.
  bool focus_block_modal_on_open_ =
      false;  ///< Set focus to first modal field on next event.

  // ===== Compositions tab state =====
  std::vector<std::string> comp_ids_;  ///< IDs shown in compositions list.
  int selected_comp_ = 0;              ///< Selected composition index.
  std::string comp_details_text_ =
      "Select a composition to see details";  ///< Right panel.
  bool show_create_comp_modal_ = false;       ///< Create composition modal.
  bool focus_comp_modal_on_open_ =
      false;  ///< Set focus to first comp modal field on next event.
  std::string comp_create_id_;           ///< New composition ID.
  std::string comp_create_block_refs_;   ///< Block refs raw input.
  std::string comp_create_static_text_;  ///< Static text fragments raw input.
  std::string comp_create_description_;  ///< New composition description.
  std::string comp_create_status_ =
      "Fill fields and press Create";  ///< Composition modal status line.
  std::vector<std::string> comp_create_fragments_preview_;  ///< List view rows.
  std::vector<std::string> comp_create_fragment_specs_;  ///< Ordered specs: B|.. T|..
  int selected_comp_fragment_ = 0;  ///< Selected fragment row in modal list.
  CompositionFragmentEditMode comp_fragment_edit_mode_ =
      CompositionFragmentEditMode::Append;  ///< How modal submit applies fragment.
  int comp_fragment_edit_index_ = -1;  ///< Target fragment index for edit/insert.
  bool show_add_block_ref_modal_ = false;   ///< Nested add block-ref modal.
  bool show_add_static_text_modal_ = false;  ///< Nested add static text modal.
  std::string comp_add_block_id_;      ///< BlockRef block ID input.
  std::string comp_add_block_version_;  ///< BlockRef version input.
  std::string comp_add_block_params_;   ///< BlockRef local params input.
  std::string comp_add_static_text_;    ///< Static text input.
  std::vector<std::string> comp_block_picker_ids_;  ///< All block IDs.
  std::vector<std::string>
      comp_block_picker_filtered_ids_;   ///< Filtered block IDs.
  std::string comp_block_picker_search_;  ///< Search query for block picker.
  int selected_comp_block_picker_ = 0;    ///< Selected block in picker.
  std::string comp_add_block_status_ = "Enter block_ref fields and submit";
  std::string comp_add_text_status_ = "Enter static text and submit";
  bool comp_edit_mode_ = false;  ///< Create modal mode: false=new, true=edit.

  // ===== Render tab state =====
  std::string render_comp_id_;  ///< Composition ID input for render tab.
  std::string render_version_;  ///< Optional composition version (major.minor).
  std::string render_params_;   ///< Raw runtime params input.
  std::string render_output_ =
      "Enter composition ID and click Render";  ///< Output.
  std::string render_status_text_ =
      "Render output actions are available after rendering.";  ///< Render status.
  bool focus_render_output_on_next_event_ = false;  ///< Deferred output focus.
  int render_focus_column_ = 1;  ///< 0=list, 1=inputs, 2=output.
  float render_output_scroll_y_ = 0.f;  ///< Output scroll position [0..1].

  // ===== Settings tab state =====
  std::string settings_project_ = "default";  ///< Active project key.
  std::string settings_path_ =
      (fs::path(sago::getConfigHome()) / "TextFoundry")
          .string();              ///< Storage path from platform config dir.
  bool settings_strict_ = false;  ///< Strict render/validation mode toggle.
  bool settings_comp_newline_delimiter_ =
      true;  ///< Apply '\n' delimiter for newly created compositions.
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
   * @brief Rebuild virtual folder list from current block IDs.
   */
  void RefreshBlockFolders();
  /**
   * @brief Rebuild filtered block list for selected folder.
   */
  void RefreshVisibleBlocks();
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
