#include "ui_style.h"

#include <ftxui/dom/elements.hpp>

namespace tf::ui {

ftxui::InputOption ModalInputOption(const bool multiline) {
  auto option = ftxui::InputOption::Default();
  option.multiline = multiline;
  option.transform = [](ftxui::InputState state) {
    state.element |= ftxui::color(ftxui::Color::White);

    if (state.is_placeholder) {
      state.element |= ftxui::dim;
    }

    if (state.focused) {
      state.element |= ftxui::bgcolor(ftxui::Color::RGB(24, 24, 28));
      state.element |= ftxui::borderStyled(ftxui::Color::GrayLight);
    } else if (state.hovered) {
      state.element |= ftxui::bgcolor(ftxui::Color::RGB(34, 34, 40));
      state.element |= ftxui::borderStyled(ftxui::Color::GrayDark);
    } else {
      state.element |= ftxui::bgcolor(ftxui::Color::Black);
      state.element |= ftxui::borderStyled(ftxui::Color::RGB(52, 52, 58));
    }

    return state.element;
  };
  return option;
}

ftxui::Element FocusedWindowTitle(const std::string& title, const bool focused) {
  auto element = ftxui::text(" " + title + " ") | ftxui::bold;
  if (focused) {
    element = element | ftxui::color(ftxui::Color::Orange1);
  }
  return element;
}

ftxui::Element ModalScrollPane(ftxui::Element content) {
  return std::move(content) | ftxui::vscroll_indicator | ftxui::yframe |
         ftxui::xflex | ftxui::yflex;
}

}  // namespace tf::ui
