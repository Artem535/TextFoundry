#include "../tui.h"
#include "tab_header.h"

#include <algorithm>
#include <cctype>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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

std::string Trim(std::string value) {
  const auto is_space = [](const unsigned char c) { return std::isspace(c); };
  while (!value.empty() && is_space(static_cast<unsigned char>(value.front()))) {
    value.erase(value.begin());
  }
  while (!value.empty() && is_space(static_cast<unsigned char>(value.back()))) {
    value.pop_back();
  }
  return value;
}

std::optional<std::string> ParseRuntimeParams(const std::string& input,
                                              Params& out) {
  std::istringstream stream(input);
  std::string line;
  while (std::getline(stream, line)) {
    line = Trim(line);
    if (line.empty()) continue;

    size_t start = 0;
    while (start <= line.size()) {
      const auto comma = line.find(',', start);
      const std::string token = line.substr(
          start, comma == std::string::npos ? std::string::npos : comma - start);
      const std::string kv = Trim(token);
      if (!kv.empty()) {
        const auto eq = kv.find('=');
        if (eq == std::string::npos) {
          return "Invalid param format: '" + kv + "' (expected key=value)";
        }
        const std::string key = Trim(kv.substr(0, eq));
        const std::string value = Trim(kv.substr(eq + 1));
        if (key.empty()) {
          return "Parameter name cannot be empty";
        }
        out[key] = value;
      }

      if (comma == std::string::npos) break;
      start = comma + 1;
    }
  }
  return std::nullopt;
}

std::optional<std::string> ParseVersion(const std::string& input,
                                        Version& out) {
  const std::string text = Trim(input);
  if (text.empty()) return std::nullopt;

  const auto dot = text.find('.');
  if (dot == std::string::npos) {
    return "Invalid version format: expected major.minor";
  }
  const std::string major_str = Trim(text.substr(0, dot));
  const std::string minor_str = Trim(text.substr(dot + 1));
  if (major_str.empty() || minor_str.empty()) {
    return "Invalid version format: expected major.minor";
  }

  try {
    const auto major_num = std::stoi(major_str);
    const auto minor_num = std::stoi(minor_str);
    if (major_num < 0 || major_num > 65535 || minor_num < 0 ||
        minor_num > 65535) {
      return "Version numbers must be in range 0..65535";
    }
    out = Version{static_cast<uint16_t>(major_num),
                  static_cast<uint16_t>(minor_num)};
  } catch (const std::exception&) {
    return "Invalid version format: expected numeric major.minor";
  }

  return std::nullopt;
}

}  // namespace

ftxui::Component Tui::RenderTab() {
  RefreshCompositionsList();
  ClampIndex(selected_comp_, static_cast<int>(comp_ids_.size()));
  if (render_comp_id_.empty() && !comp_ids_.empty()) {
    render_comp_id_ = comp_ids_[selected_comp_];
  }

  auto compositions_menu = ftxui::Menu(&comp_ids_, &selected_comp_);
  auto comp_id_input =
      ftxui::Input(&render_comp_id_, "composition.id (e.g. welcome.message)");
  auto version_input =
      ftxui::Input(&render_version_, "optional version, e.g. 1.0");
  auto params_input =
      ftxui::Input(&render_params_, "name=value, other=123 (comma or newline)");
  auto output_component = ftxui::Renderer([this](const bool focused) {
    render_output_scroll_y_ = std::clamp(render_output_scroll_y_, 0.f, 1.f);
    auto output_content =
        ftxui::hflow(ftxui::paragraph(render_output_)) |
        ftxui::focusPositionRelative(0.f, render_output_scroll_y_) |
        ftxui::vscroll_indicator | ftxui::yframe | ftxui::xflex | ftxui::yflex;
    if (focused) {
      output_content = output_content | ftxui::focus;
    }
    return ftxui::window(ftxui::text(" Output "), output_content) |
           ftxui::xflex | ftxui::yflex;
  });
  constexpr float kArrowStep = 0.04f;
  constexpr float kPageStep = 0.20f;
  const auto handle_output_scroll_event = [this](ftxui::Event e) {
    const auto apply_scroll = [this](const float delta) {
      render_output_scroll_y_ =
          std::clamp(render_output_scroll_y_ + delta, 0.f, 1.f);
    };
    if (e == ftxui::Event::ArrowDown) {
      apply_scroll(kArrowStep);
      return true;
    }
    if (e == ftxui::Event::ArrowUp) {
      apply_scroll(-kArrowStep);
      return true;
    }
    if (e == ftxui::Event::PageDown) {
      apply_scroll(kPageStep);
      return true;
    }
    if (e == ftxui::Event::PageUp) {
      apply_scroll(-kPageStep);
      return true;
    }
    if (e == ftxui::Event::Home) {
      render_output_scroll_y_ = 0.f;
      return true;
    }
    if (e == ftxui::Event::End) {
      render_output_scroll_y_ = 1.f;
      return true;
    }
    if (e.is_mouse()) {
      if (e.mouse().button == ftxui::Mouse::WheelDown) {
        apply_scroll(kArrowStep);
        return true;
      }
      if (e.mouse().button == ftxui::Mouse::WheelUp) {
        apply_scroll(-kArrowStep);
        return true;
      }
    }
    return false;
  };
  output_component = output_component | ftxui::CatchEvent(handle_output_scroll_event);

  auto render_button = ftxui::Button("Render", [this, output_component] {
    if (render_comp_id_.empty()) {
      render_output_ = "Error: Composition ID is required.";
      render_output_scroll_y_ = 0.f;
      focus_render_output_on_next_event_ = true;
      render_focus_column_ = 2;
      if (auto* active = ftxui::ScreenInteractive::Active(); active != nullptr) {
        active->PostEvent(ftxui::Event::Custom);
      }
      return;
    }

    RenderContext ctx;
    ctx.strictMode = settings_strict_;
    if (const auto parse_err = ParseRuntimeParams(render_params_, ctx.params);
        parse_err.has_value()) {
      render_output_ = "Error: " + *parse_err;
      render_output_scroll_y_ = 0.f;
      focus_render_output_on_next_event_ = true;
      render_focus_column_ = 2;
      if (auto* active = ftxui::ScreenInteractive::Active(); active != nullptr) {
        active->PostEvent(ftxui::Event::Custom);
      }
      return;
    }

    Version version;
    if (const auto parse_err = ParseVersion(render_version_, version);
        parse_err.has_value()) {
      render_output_ = "Error: " + *parse_err;
      render_output_scroll_y_ = 0.f;
      focus_render_output_on_next_event_ = true;
      render_focus_column_ = 2;
      if (auto* active = ftxui::ScreenInteractive::Active(); active != nullptr) {
        active->PostEvent(ftxui::Event::Custom);
      }
      return;
    }

    auto result = Trim(render_version_).empty()
                      ? engine_.Render(render_comp_id_, ctx)
                      : engine_.Render(render_comp_id_, version, ctx);
    if (result.HasError()) {
      render_output_ = "Error: " + result.error().message;
      render_output_scroll_y_ = 0.f;
      focus_render_output_on_next_event_ = true;
      render_focus_column_ = 2;
      if (auto* active = ftxui::ScreenInteractive::Active(); active != nullptr) {
        active->PostEvent(ftxui::Event::Custom);
      }
      return;
    }

    render_output_ = result.value().text;
    render_output_scroll_y_ = 0.f;
    focus_render_output_on_next_event_ = true;
    render_focus_column_ = 2;
    if (auto* active = ftxui::ScreenInteractive::Active(); active != nullptr) {
      active->PostEvent(ftxui::Event::Custom);
    }
  });

  auto clear_button = ftxui::Button("Clear", [this] {
    render_params_.clear();
    render_output_ = "Enter composition ID and click Render";
    render_output_scroll_y_ = 0.f;
  });

  auto refresh_button = ftxui::Button("Refresh list", [this] {
    const std::string selected_id = render_comp_id_;
    RefreshCompositionsList();
    if (!selected_id.empty()) {
      const auto it = std::ranges::find(comp_ids_, selected_id);
      if (it != comp_ids_.end()) {
        selected_comp_ =
            static_cast<int>(std::distance(comp_ids_.begin(), it));
      }
    }
    ClampIndex(selected_comp_, static_cast<int>(comp_ids_.size()));
  });

  auto controls = ftxui::Container::Vertical(
      {comp_id_input, version_input, params_input, render_button, clear_button,
       refresh_button});

  auto main = ftxui::Container::Horizontal(
      {compositions_menu, controls, output_component}, &render_focus_column_);
  main = main | ftxui::CatchEvent(
                    [this, output_component, compositions_menu,
                     handle_output_scroll_event](
                        const ftxui::Event& e) {
           if (focus_render_output_on_next_event_) {
             render_focus_column_ = 2;
             output_component->TakeFocus();
             focus_render_output_on_next_event_ = false;
           }
           if (render_focus_column_ == 2 && handle_output_scroll_event(e)) {
             return true;
           }
           if (IsCompositionListEvent(e) && compositions_menu->Focused()) {
             ClampIndex(selected_comp_, static_cast<int>(comp_ids_.size()));
             if (!comp_ids_.empty()) {
               render_comp_id_ = comp_ids_[selected_comp_];
             }
           }
           if (e == ftxui::Event::CtrlL) {
             render_output_ = "Enter composition ID and click Render";
             render_output_scroll_y_ = 0.f;
             return true;
           }
           return false;
         });

  return ftxui::Renderer(main, [compositions_menu, comp_id_input, version_input,
                                params_input, render_button, clear_button,
                                refresh_button, output_component, this] {
    auto list_panel = ftxui::window(
                          ftxui::text(" Compositions "),
                          compositions_menu->Render() | ftxui::flex) |
                      ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 36) |
                      ftxui::yflex;

    auto controls_panel = ftxui::window(
        ftxui::text(" Render Inputs "),
        ftxui::vbox({
            ftxui::text("Composition ID"),
            comp_id_input->Render(),
            ftxui::separator(),
            ftxui::text("Version (optional)"),
            version_input->Render(),
            ftxui::text("Leave empty to render latest version") | ftxui::dim,
            ftxui::separator(),
            ftxui::text("Runtime params"),
            params_input->Render(),
            ftxui::text("Format: key=value, key2=value2") | ftxui::dim,
            ftxui::separator(),
            render_button->Render(),
            clear_button->Render(),
            refresh_button->Render(),
            ftxui::separator(),
            ftxui::text(settings_strict_ ? "Strict mode: ON"
                                         : "Strict mode: OFF") |
                ftxui::dim,
        })) |
                          ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 34) |
                          ftxui::yflex;

    return ftxui::hbox({
               list_panel,
               controls_panel,
               output_component->Render() | ftxui::xflex | ftxui::yflex,
           }) |
           ftxui::xflex | ftxui::yflex;
  });
}

}  // namespace tf
