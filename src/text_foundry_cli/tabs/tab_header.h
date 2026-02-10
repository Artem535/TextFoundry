#pragma once

#include <ftxui/dom/elements.hpp>

#include <string>

namespace tf::ui {

inline ftxui::Element MakeHeader(
    std::string text, ftxui::Color color = ftxui::Color::White) {
  return ftxui::text(std::move(text)) | ftxui::bold | ftxui::color(color) |
         ftxui::center;
}

}  // namespace tf::ui
