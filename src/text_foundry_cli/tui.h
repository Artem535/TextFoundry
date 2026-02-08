//
// Created by artem.d on 06.02.2026.
//

#pragma once

#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp>

#include <tf/engine.h>

namespace tf {
  class Tui {
  public:
    explicit Tui();
    ftxui::Component render();
    void run();

  private:
    ftxui::Component root_component_;
    ftxui::ScreenInteractive screen_;
  };
} // tf
