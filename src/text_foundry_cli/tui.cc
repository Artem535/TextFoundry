//
// Created by artem.d on 06.02.2026.
//

#include "tui.h"

namespace tf {
  Tui::Tui() : screen_{ftxui::ScreenInteractive::Fullscreen()} {
  }

  ftxui::Component Tui::render() {
    const auto render = ftxui::Renderer(
      []() {
        return ftxui::vbox(
                 ftxui::text("Text Foundry"),
                 ftxui::text("Press F10 key to exit")
               ) | ftxui::center | ftxui::border;
      }
    );

    auto navigation = ftxui::CatchEvent(render, [this](const ftxui::Event &event) {
      if (event == ftxui::Event::F10) {
        screen_.ExitLoopClosure()();
        return true;
      }
      return false;
    });

    return navigation;
  }

  void Tui::run() {
    screen_.Loop(render());
  }
} // tf
