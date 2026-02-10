#include "../tui.h"
#include "tab_header.h"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

namespace tf {

ftxui::Component Tui::SettingsTab() {
  auto root = ftxui::Container::Vertical({});
  return ftxui::Renderer(root, [] {
    return ftxui::vbox({
               ui::MakeHeader("Settings", ftxui::Color::Yellow),
               ftxui::separator(),
               ftxui::text("Temporary stub tab"),
               ftxui::text("Diagnostics mode: component content disabled") |
                   ftxui::dim,
           }) |
           ftxui::flex | ftxui::border;
  });
}

}  // namespace tf
