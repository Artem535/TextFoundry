#pragma once

#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>

#include <string>

namespace tf::ui {

ftxui::InputOption ModalInputOption(bool multiline = false);
ftxui::Element FocusedWindowTitle(const std::string& title, bool focused);
ftxui::Element ModalScrollPane(ftxui::Element content);

}  // namespace tf::ui
