#include "../tui.h"
#include "tab_header.h"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

namespace tf {

ftxui::Component Tui::RenderTab() {
  auto root = ftxui::Container::Vertical({});
  return ftxui::Renderer(root, [] {
    return ftxui::vbox({
               ui::MakeHeader("Render", ftxui::Color::Magenta),
               ftxui::separator(),
               ftxui::text("Temporary stub tab"),
               ftxui::text("Diagnostics mode: component content disabled") |
                   ftxui::dim,
           }) |
           ftxui::flex | ftxui::border;
  });
}

}  // namespace tf
