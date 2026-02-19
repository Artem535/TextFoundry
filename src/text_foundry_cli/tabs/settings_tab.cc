#include "../tui.h"
#include "tab_header.h"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

namespace tf {

ftxui::Component Tui::SettingsTab() {
  auto project_input = ftxui::Input(&settings_project_, "default");
  auto path_input = ftxui::Input(&settings_path_, "/path/to/TextFoundry");
  auto strict_checkbox = ftxui::Checkbox("Enable strict mode", &settings_strict_);

  auto apply_button = ftxui::Button("Apply", [this] {
    settings_status_text_ =
        "Applied. Strict mode now affects Render tab requests.";
  });
  auto reset_button = ftxui::Button("Reset defaults", [this] {
    settings_project_ = "default";
    settings_path_ = (fs::path(sago::getConfigHome()) / "TextFoundry").string();
    settings_strict_ = false;
    settings_status_text_ = "Defaults restored for this TUI session.";
  });

  auto root = ftxui::Container::Vertical(
      {project_input, path_input, strict_checkbox, apply_button, reset_button});
  return ftxui::Renderer(
      root, [project_input, path_input, strict_checkbox, apply_button,
             reset_button, this] {
    return ftxui::vbox({
               ui::MakeHeader("Settings", ftxui::Color::Yellow),
               ftxui::separator(),
               ftxui::window(ftxui::text(" Session "),
                             ftxui::vbox({
                                 ftxui::text("Project key"),
                                 project_input->Render(),
                                 ftxui::separator(),
                                 ftxui::text("Data path"),
                                 path_input->Render(),
                                 ftxui::separator(),
                                 strict_checkbox->Render(),
                                 ftxui::separator(),
                                 apply_button->Render(),
                                 reset_button->Render(),
                             })) |
                   ftxui::xflex,
               ftxui::separator(),
               ftxui::text(settings_status_text_) | ftxui::dim,
           }) |
           ftxui::xflex | ftxui::yflex | ftxui::border;
  });
}

}  // namespace tf
