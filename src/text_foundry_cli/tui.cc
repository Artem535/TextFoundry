//
// Simplified FTXUI implementation (single file)
//

#include "tui.h"
#include <tf/engine.h>

#include <fmt/format.h>

#include <cstdlib>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iostream>
#include <string_view>
#include <unistd.h>

namespace tf {
  // ============================================================================
  // Helpers
  // ============================================================================

  static ftxui::Element header_text(std::string text, ftxui::Color c = ftxui::Color::White) {
    return ftxui::text(text) | ftxui::bold | ftxui::color(c) | ftxui::center;
  }


  static bool is_user_input_event(const ftxui::Event &e) {
    if (e.is_mouse()) return false;
    if (e.is_cursor_position()) return false;
    if (e.is_cursor_shape()) return false;
    if (e == ftxui::Event::Custom) return false;
    return true;
  }

  static bool is_exit_event(const ftxui::Event &e) {
    // F10 mapping can vary by terminal. Support both FTXUI symbolic event
    // and common raw CSI sequence for F10.
    if (e == ftxui::Event::F10) return true;
    if (e.input() == "\x1b[21~") return true; // F10 in many terminals
    if (e == ftxui::Event::Escape) return true;
    if (e == ftxui::Event::Character('q')) return true;
    return false;
  }

  static void safe_clamp_index(int &idx, int size) {
    if (size <= 0) {
      idx = 0;
      return;
    }
    if (idx < 0) idx = 0;
    if (idx >= size) idx = size - 1;
  }

  // ============================================================================
  // Constructor & Main Loop
  // ============================================================================

  Tui::Tui(Engine &engine) : engine_(engine) {
  }

  void Tui::run() {
    // FTXUI fullscreen mode needs a real interactive terminal.
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
      std::cerr << "Error: TUI requires an interactive terminal (TTY). "
          "Run `tf tui` in a real terminal emulator.\n";
      return;
    }

    const char *term = std::getenv("TERM");
    if (term == nullptr || std::string_view(term) == "dumb") {
      std::cerr << "Error: TUI requires TERM with cursor support "
          "(for example: xterm-256color).\n";
      return;
    }

    screen_.Loop(main_layout());
  }

  // ============================================================================
  // Main Layout
  // ============================================================================

ftxui::Component Tui::main_layout() {
  // --- Tabs
  auto tab_toggle = ftxui::Toggle(&tab_names_, &selected_tab_);
  tab_toggle->TakeFocus();

  auto tab_contents = ftxui::Container::Tab(
    {
      blocks_tab(),
      compositions_tab(),
      render_tab(),
      settings_tab(),
    },
    &selected_tab_);

  // Root container: именно он правильно раздаёт размеры детям
  auto root = ftxui::Container::Vertical({
    tab_toggle,
    tab_contents,
  });

  // --- Main renderer: рендерим root->Render(), а не детей по отдельности
  auto main_renderer = ftxui::Renderer(root, [&] {
    auto title =
      ftxui::text(" TextFoundry ")
      | ftxui::bold
      | ftxui::center;

    auto footer =
      ftxui::text(" F10/Esc/q = Exit ")
      | ftxui::dim
      | ftxui::center;

    // ВАЖНО:
    // 1) root->Render() | flex, чтобы занял центральную область.
    // 2) весь vbox | flex, чтобы занял весь экран.
    auto layout =
      ftxui::vbox({
        title,
        ftxui::separator(),
        root->Render() | ftxui::flex,
        ftxui::separator(),
        footer,
      })
      | ftxui::border
      | ftxui::flex;

    return layout;
  });

  // --- Welcome modal (если show_welcome_ = true)
  auto modal = ftxui::Renderer([&] {
    return ftxui::vbox({
             ftxui::text("TextFoundry TUI") | ftxui::bold | ftxui::center,
             ftxui::separator(),
             ftxui::text("F10 - Exit  |  Any key - Continue") | ftxui::dim | ftxui::center,
           })
           | ftxui::border
           | ftxui::center
           | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 44)
           | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 9);
  });

  auto with_modal = ftxui::Modal(main_renderer, modal, &show_welcome_);

  // --- Global event handler
  return ftxui::CatchEvent(with_modal, [this, root](const ftxui::Event& e) {
    if (is_exit_event(e)) {
      screen_.ExitLoopClosure()();
      return true;
    }

    if (show_welcome_ && is_user_input_event(e)) {
      show_welcome_ = false;
      root->TakeFocus();
      return true;
    }

    return false;
  });
}


  // ============================================================================
  // Blocks Tab
  // ============================================================================

  ftxui::Component Tui::blocks_tab() {
    return ftxui::Renderer([] {
      return ftxui::text("block_tab");
    });
  }

  // ============================================================================
  // Compositions Tab
  // ============================================================================

  ftxui::Component Tui::compositions_tab() {
    return ftxui::Renderer([] {
      return ftxui::text("Composition_tab");
    });
  }

  // ============================================================================
  // Render Tab
  // ============================================================================

  ftxui::Component Tui::render_tab() {
    return ftxui::Renderer([] {
      return ftxui::text("Render_tab");
    });
  }

  // ============================================================================
  // Settings Tab
  // ============================================================================

  ftxui::Component Tui::settings_tab() {
    return ftxui::Renderer([] {
      return ftxui::text("Setting_tab");
    });
  }
} // namespace tf
