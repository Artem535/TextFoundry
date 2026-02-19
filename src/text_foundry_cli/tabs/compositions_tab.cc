#include "../tui.h"
#include "tab_header.h"

#include <algorithm>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <sstream>

namespace tf {
namespace {
void ClampIndex(int& idx, const int size) {
  if (size <= 0) {
    idx = 0;
    return;
  }
  idx = std::max(0, std::min(idx, size - 1));
}

bool IsCompositionListEvent(const ftxui::Event& e) {
  return e == ftxui::Event::ArrowUp || e == ftxui::Event::ArrowDown ||
         e == ftxui::Event::Return || e.is_mouse();
}

std::string PreviewText(const std::string& text, const size_t max_len = 72) {
  std::string one_line = text;
  std::ranges::replace(one_line, '\n', ' ');
  if (one_line.size() <= max_len) return one_line;
  return one_line.substr(0, max_len - 3) + "...";
}
}  // namespace

void Tui::RefreshCompositionsList() {
  comp_ids_ = engine_.ListCompositions();
  ClampIndex(selected_comp_, static_cast<int>(comp_ids_.size()));
}

void Tui::UpdateSelectedCompositionDetails() {
  if (comp_ids_.empty()) {
    comp_details_text_ = "No compositions found.";
    return;
  }

  auto result = engine_.LoadComposition(comp_ids_[selected_comp_]);
  if (result.HasError()) {
    comp_details_text_ = "Error: " + result.error().message;
    return;
  }

  const auto& comp = result.value();
  std::ostringstream out;
  out << "ID: " << comp.id() << "\n";
  out << "Version: " << comp.version().ToString() << "\n";
  out << "State: " << BlockStateToString(comp.state()) << "\n";
  out << "Fragments: " << comp.fragmentCount() << "\n";

  if (!comp.description().empty()) {
    out << "Description: " << comp.description() << "\n";
  }

  out << "\nFragment list:\n";
  size_t idx = 0;
  for (const auto& fragment : comp.fragments()) {
    out << "  " << idx++ << ". ";
    if (fragment.IsBlockRef()) {
      const auto& ref = fragment.AsBlockRef();
      out << "BlockRef " << ref.GetBlockId();
      if (ref.version().has_value()) {
        out << "@" << ref.version()->ToString();
      } else {
        out << "@latest";
      }
      if (!ref.LocalParams().empty()) {
        out << " (params: " << ref.LocalParams().size() << ")";
      }
      out << "\n";
      continue;
    }

    if (fragment.IsStaticText()) {
      out << "StaticText: \"" << PreviewText(fragment.AsStaticText().text())
          << "\"\n";
      continue;
    }

    out << "Separator: "
        << SeparatorTypeToString(fragment.AsSeparator().type) << "\n";
  }

  comp_details_text_ = out.str();
}

ftxui::Component Tui::CompositionsTab() {
  RefreshCompositionsList();
  UpdateSelectedCompositionDetails();

  auto list = ftxui::Menu(&comp_ids_, &selected_comp_);
  auto refresh_button = ftxui::Button("Refresh", [this] {
    RefreshCompositionsList();
    UpdateSelectedCompositionDetails();
  });

  auto details = ftxui::Renderer([this] {
    return ftxui::window(
               ftxui::text(" Details "),
               ftxui::paragraph(comp_details_text_) | ftxui::xflex |
                   ftxui::yflex) |
           ftxui::xflex | ftxui::yflex;
  });

  auto content = ftxui::Container::Horizontal({list, details, refresh_button});
  content = content | ftxui::CatchEvent([this](const ftxui::Event& e) {
              if (IsCompositionListEvent(e)) {
                ClampIndex(selected_comp_, static_cast<int>(comp_ids_.size()));
                UpdateSelectedCompositionDetails();
              }
              return false;
            });

  auto body = ftxui::Renderer(content, [list, details, refresh_button] {
    auto left = ftxui::window(ftxui::text(" Compositions "),
                              list->Render() | ftxui::flex) |
                ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 36) | ftxui::yflex;
    auto actions = ftxui::window(ftxui::text(" Actions "),
                                 refresh_button->Render() | ftxui::flex) |
                   ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 24) |
                   ftxui::yflex;

    return ftxui::vbox({
               ui::MakeHeader("Compositions", ftxui::Color::Green),
               ftxui::separator(),
               ftxui::hbox({
                   left,
                   details->Render() | ftxui::xflex | ftxui::yflex,
                   actions,
               }) |
                   ftxui::xflex | ftxui::yflex,
           }) |
           ftxui::xflex | ftxui::yflex | ftxui::border;
  });

  return body;
}

}  // namespace tf
