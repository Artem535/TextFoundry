//
// Simplified FTXUI-based TUI for TextFoundry
// Single translation unit (tui.cc)
//
// Key rules implemented here:
//  - Never swallow all events while a modal is visible (FTXUI needs internal
//  events).
//  - Always give the UI a focus target (otherwise keyboard feels "dead").
//  - Ensure the central area gets height (use | flex).
//  - Force redraw on terminal resize (post Event::Custom).
//

#include "tui.h"

#include <fmt/format.h>
#include <tf/engine.h>
#include <unistd.h>  // isatty, STDIN_FILENO, STDOUT_FILENO

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "tf/obx_utils.hpp"

namespace tf {
namespace {
// ----------------------------------------------------------------------------
// Small UI helpers
// ----------------------------------------------------------------------------

ftxui::Element make_header(std::string text,
                           ftxui::Color c = ftxui::Color::White) {
  return ftxui::text(std::move(text)) | ftxui::bold | ftxui::color(c) |
         ftxui::center;
}

// Consider "user input" as any non-mouse, non-cursor, non-custom event.
// This includes arrows, enter, function keys, etc.
bool IsUserInputEvent(const ftxui::Event& e) {
  if (e.is_mouse()) return false;
  if (e.is_cursor_position()) return false;
  if (e.is_cursor_shape()) return false;
  if (e == ftxui::Event::Custom) return false;
  return true;
}

// Exit shortcuts. F10 mapping can vary between terminals; Esc and 'q' are
// reliable.
bool IsExitEvent(const ftxui::Event& e) {
  if (e == ftxui::Event::F10) return true;
  if (e == ftxui::Event::Escape) return true;
  if (e == ftxui::Event::Character('q')) return true;
  // Common CSI sequence for F10 in many terminals.
  if (e.input() == "\x1b[21~") return true;
  return false;
}

void ClampIndex(int& idx, int size) {
  if (size <= 0) {
    idx = 0;
    return;
  }
  idx = std::max(0, std::min(idx, size - 1));
}

inline void ltrim(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

inline void rtrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

inline void trim(std::string& s) {
  ltrim(s);
  rtrim(s);
}

// Parse "key=value, key2=value2" into ctx.params.
// Spaces around keys are trimmed; values are kept as-is (except surrounding
// spaces).
void ParseParamsInto(tf::RenderContext& ctx, const std::string& raw) {
  std::size_t pos = 0;
  while (pos < raw.size()) {
    // Skip separators/spaces
    while (pos < raw.size() && (raw[pos] == ' ' || raw[pos] == ',')) pos++;
    if (pos >= raw.size()) break;

    const std::size_t eq = raw.find('=', pos);
    if (eq == std::string::npos) break;

    std::size_t comma = raw.find(',', eq + 1);
    if (comma == std::string::npos) comma = raw.size();

    std::string key = raw.substr(pos, eq - pos);
    std::string val = raw.substr(eq + 1, comma - (eq + 1));
    trim(key);
    trim(val);

    if (!key.empty()) ctx.params[key] = val;

    pos = comma + 1;
  }
}
}  // namespace

// ============================================================================
// Constructor & Main Loop
// ============================================================================

Tui::Tui(Engine& engine) : engine_(engine) {}

void Tui::run() {
  // Fullscreen mode needs a real interactive terminal.
  // If you prefer not to enforce this, you can remove the checks.
  if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
    std::cerr << "Error: TUI requires an interactive terminal (TTY).\n"
              << "Run: tf tui (from a real terminal emulator)\n";
    return;
  }

  const char* term = std::getenv("TERM");
  if (term == nullptr || std::string_view(term) == "dumb") {
    std::cerr << "Error: TERM is not set (or set to 'dumb'). "
              << "Use a terminal with cursor support (e.g., xterm-256color).\n";
    return;
  }

  const char* force_terminal_output = std::getenv("TF_TUI_TERMINAL_OUTPUT");
  const bool use_terminal_output =
      force_terminal_output != nullptr &&
      std::string_view(force_terminal_output) == "1";

  // Fullscreen is the default mode.
  // TerminalOutput is available as explicit fallback.
  auto screen = use_terminal_output ? ftxui::ScreenInteractive::TerminalOutput()
                                    : ftxui::ScreenInteractive::Fullscreen();

  auto root = MainLayout(screen);
  // Force first repaint to avoid occasional "blank frame" on startup.
  screen.PostEvent(ftxui::Event::Custom);
  screen.Loop(root);
}

// ============================================================================
// Main Layout
// ============================================================================

ftxui::Component Tui::MainLayout(auto& screen) {
  auto tab_toggle = ftxui::Toggle(&tab_names_, &selected_tab_);
  auto tab_container = ftxui::Container::Tab(
      {
          BlocksTab(),
          CompositionsTab(),
          RenderTab(),
          SettingsTab(),
      },
      &selected_tab_);

  auto container = ftxui::Container::Vertical({tab_toggle, tab_container});
  tab_toggle->TakeFocus();

  auto layout = ftxui::Renderer(container, [tab_toggle, tab_container, this] {
    return ftxui::vbox({
               ftxui::text("TextFoundry") | ftxui::bold |
                   ftxui::color(ftxui::Color::Orange1),
               ftxui::separator(),
               tab_toggle->Render(),
               ftxui::separator(),
               tab_container->Render() | ftxui::flex,
               ftxui::separator(),
               ftxui::text("q / Esc / F10 = Exit") | ftxui::dim | ftxui::center,
           }) |
           ftxui::border | ftxui::flex;
  });

  auto modal = ftxui::Renderer([] {
    return ftxui::vbox({
               ftxui::text("TextFoundry TUI") | ftxui::bold | ftxui::center |
                   ftxui::color(ftxui::Color::Orange1),
               ftxui::separator(),
               ftxui::text("Any key - continue") | ftxui::dim | ftxui::center,
               ftxui::text("F10 / Esc / q - exit") | ftxui::dim | ftxui::center,
           }) |
           ftxui::border | ftxui::center |
           ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 44) |
           ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 9);
  });

  auto with_modal = ftxui::Modal(layout, modal, &show_welcome_);

  return ftxui::CatchEvent(with_modal, [&, tab_toggle](const ftxui::Event& e) {
    if (IsExitEvent(e)) {
      screen.ExitLoopClosure()();
      return true;
    }
    if (show_welcome_ && IsUserInputEvent(e)) {
      show_welcome_ = false;
      tab_toggle->TakeFocus();
      return true;
    }
    return false;
  });
}

// ============================================================================
// Blocks Tab
// ============================================================================

ftxui::Component Tui::BlocksTab() {
  block_ids_ = engine_.ListBlocks();
  ClampIndex(selected_block_, static_cast<int>(block_ids_.size()));

  auto update_details = [this] {
    if (block_ids_.empty()) {
      details_text_ = "No blocks found.";
      return;
    }
    auto result = engine_.LoadBlock(block_ids_[selected_block_]);
    if (result.HasValue()) {
      const auto& b = result.value();
      details_text_ =
          fmt::format("ID: {}\nVersion: {}.{}\nType: {}\nTemplate: {}", b.Id(),
                      b.version().major, b.version().minor,
                      BlockTypeToString(b.type()), b.templ().Сontent());
    } else {
      details_text_ = "Error: " + result.error().message;
    }
  };

  update_details();

  auto list = ftxui::Menu(&block_ids_, &selected_block_);
  auto details = ftxui::Renderer([this] {
    return ftxui::window(ftxui::text(" Details "),
                         ftxui::paragraph(details_text_) | ftxui::flex) |
           ftxui::flex;
  });

  auto content = ftxui::Container::Horizontal({list, details});
  content =
      content |
      ftxui::CatchEvent([this, update_details](const ftxui::Event& e) {
        if (e == ftxui::Event::ArrowUp || e == ftxui::Event::ArrowDown ||
            e == ftxui::Event::Return || e.is_mouse()) {
          ClampIndex(selected_block_, static_cast<int>(block_ids_.size()));
          update_details();
        }
        return false;
      });

  return ftxui::Renderer(content, [list, details] {
    auto left =
        ftxui::window(ftxui::text(" Blocks "), list->Render() | ftxui::flex) |
        ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 42) | ftxui::flex;

    return ftxui::hbox({
               left,
               ftxui::separator(),
               details->Render() | ftxui::flex,
           }) |
           ftxui::flex;
  });
}

// ============================================================================
// Compositions Tab
// ============================================================================

ftxui::Component Tui::CompositionsTab() {
  auto root = ftxui::Container::Vertical({});
  return ftxui::Renderer(root, [] {
    return ftxui::vbox({
               make_header("Compositions", ftxui::Color::Green),
               ftxui::separator(),
               ftxui::text("Temporary stub tab"),
               ftxui::text("Diagnostics mode: component content disabled") |
                   ftxui::dim,
           }) |
           ftxui::flex | ftxui::border;
  });
}

// ============================================================================
// Render Tab
// ============================================================================

ftxui::Component Tui::RenderTab() {
  auto root = ftxui::Container::Vertical({});
  return ftxui::Renderer(root, [] {
    return ftxui::vbox({
               make_header("Render", ftxui::Color::Magenta),
               ftxui::separator(),
               ftxui::text("Temporary stub tab"),
               ftxui::text("Diagnostics mode: component content disabled") |
                   ftxui::dim,
           }) |
           ftxui::flex | ftxui::border;
  });
}

// ============================================================================
// Settings Tab
// ============================================================================

ftxui::Component Tui::SettingsTab() {
  auto root = ftxui::Container::Vertical({});
  return ftxui::Renderer(root, [] {
    return ftxui::vbox({
               make_header("Settings", ftxui::Color::Yellow),
               ftxui::separator(),
               ftxui::text("Temporary stub tab"),
               ftxui::text("Diagnostics mode: component content disabled") |
                   ftxui::dim,
           }) |
           ftxui::flex | ftxui::border;
  });
}
}  // namespace tf
