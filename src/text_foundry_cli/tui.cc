//
// Simplified FTXUI-based TUI for TextFoundry
//

#include "tui.h"

#include <cstdlib>
#include <cstdio>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <iostream>
#include <string_view>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace tf {
namespace {
bool HasInteractiveTerminal() {
#if defined(_WIN32)
  return _isatty(_fileno(stdin)) && _isatty(_fileno(stdout));
#else
  return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
#endif
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

// Exit shortcuts. Keep explicit F10 handling for portability.
bool IsExitEvent(const ftxui::Event& e) {
  if (e == ftxui::Event::F10) return true;
  // Common CSI sequence for F10 in many terminals.
  if (e.input() == "\x1b[21~") return true;
  return false;
}
}  // namespace

Tui::Tui(Engine& engine) : engine_(engine), block_creator_(engine) {}

void Tui::Run() {
  // Fullscreen mode needs a real interactive terminal.
  if (!HasInteractiveTerminal()) {
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

  auto screen = [&] {
    if (use_terminal_output) {
      return ftxui::ScreenInteractive::TerminalOutput();
    }
    return ftxui::ScreenInteractive::Fullscreen();
  }();

  auto root = MainLayout(screen);
  screen.PostEvent(ftxui::Event::Custom);
  screen.Loop(root);
}

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
               ftxui::text("F10 = Exit") | ftxui::dim | ftxui::center,
           }) |
           ftxui::border | ftxui::flex;
  });

  auto modal = ftxui::Renderer([] {
    return ftxui::vbox({
               ftxui::text("TextFoundry TUI") | ftxui::bold | ftxui::center |
                   ftxui::color(ftxui::Color::Orange1),
               ftxui::separator(),
               ftxui::text("Any key - continue") | ftxui::dim | ftxui::center,
               ftxui::text("F10 - exit") | ftxui::dim | ftxui::center,
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

}  // namespace tf
