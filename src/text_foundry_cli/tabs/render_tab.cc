#include "../tui.h"
#include "../ui_style.h"
#include "tab_header.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iostream>
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

std::string Base64Encode(const std::string& input) {
  static constexpr char kAlphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  std::string output;
  output.reserve(((input.size() + 2) / 3) * 4);

  for (size_t i = 0; i < input.size(); i += 3) {
    const auto b0 = static_cast<std::uint8_t>(input[i]);
    const auto b1 = i + 1 < input.size() ? static_cast<std::uint8_t>(input[i + 1]) : 0;
    const auto b2 = i + 2 < input.size() ? static_cast<std::uint8_t>(input[i + 2]) : 0;

    output.push_back(kAlphabet[(b0 >> 2) & 0x3F]);
    output.push_back(kAlphabet[((b0 & 0x03) << 4) | ((b1 >> 4) & 0x0F)]);
    output.push_back(i + 1 < input.size()
                         ? kAlphabet[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)]
                         : '=');
    output.push_back(i + 2 < input.size() ? kAlphabet[b2 & 0x3F] : '=');
  }

  return output;
}

bool CopyToClipboardOsc52(const std::string& text) {
  if (text.empty()) {
    return false;
  }

  std::cout << "\033]52;c;" << Base64Encode(text) << "\a" << std::flush;
  return static_cast<bool>(std::cout);
}

StructuralStyle EffectiveStyle(const Composition& composition) {
  if (composition.GetStyleProfile().has_value()) {
    return composition.GetStyleProfile()->structural;
  }
  return {};
}

std::string ApplyStructuralStyle(const std::vector<std::string>& fragment_texts,
                                 const StructuralStyle& style) {
  std::ostringstream result;
  if (style.preamble.has_value()) {
    result << *style.preamble;
  }

  for (size_t i = 0; i < fragment_texts.size(); ++i) {
    std::string text = fragment_texts[i];
    if (style.blockWrapper.has_value()) {
      std::string wrapped = *style.blockWrapper;
      constexpr std::string_view kContent = "{{content}}";
      if (const size_t pos = wrapped.find(kContent); pos != std::string::npos) {
        wrapped.replace(pos, kContent.size(), text);
      }
      text = std::move(wrapped);
    }

    result << text;
    if (i + 1 < fragment_texts.size() && style.delimiter.has_value()) {
      result << *style.delimiter;
    }
  }

  if (style.postamble.has_value()) {
    result << *style.postamble;
  }
  return result.str();
}

Result<std::string> BuildRawRender(Engine& engine, const Composition& composition) {
  std::vector<std::string> fragment_texts;
  fragment_texts.reserve(composition.fragments().size());

  for (const auto& fragment : composition.fragments()) {
    if (fragment.IsStaticText()) {
      fragment_texts.push_back(fragment.AsStaticText().text());
      continue;
    }
    if (fragment.IsSeparator()) {
      fragment_texts.push_back(fragment.AsSeparator().toString());
      continue;
    }

    const auto& block_ref = fragment.AsBlockRef();
    auto block_result = block_ref.version().has_value()
                            ? engine.LoadBlock(block_ref.GetBlockId(),
                                               *block_ref.version())
                            : engine.LoadBlock(block_ref.GetBlockId());
    if (block_result.HasError()) {
      return Result<std::string>(block_result.error());
    }
    fragment_texts.push_back(block_result.value().templ().Content());
  }

  return Result<std::string>(
      ApplyStructuralStyle(fragment_texts, EffectiveStyle(composition)));
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
    return ftxui::window(ui::FocusedWindowTitle("Output", focused),
                         output_content) |
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
    render_status_text_ = "Rendered successfully.";
    focus_render_output_on_next_event_ = true;
    render_focus_column_ = 2;
    if (auto* active = ftxui::ScreenInteractive::Active(); active != nullptr) {
      active->PostEvent(ftxui::Event::Custom);
    }
  });

  auto copy_render_button = ftxui::Button("Copy Render", [this] {
    if (render_output_.empty() ||
        render_output_ == "Enter composition ID and click Render" ||
        render_output_.starts_with("Error: ")) {
      render_status_text_ = "Nothing to copy from render output.";
      return;
    }

    render_status_text_ = CopyToClipboardOsc52(render_output_)
                              ? "Copied render output to terminal clipboard."
                              : "Failed to copy render output.";
  });

  auto copy_raw_button = ftxui::Button("Copy Raw", [this] {
    if (render_comp_id_.empty()) {
      render_status_text_ = "Composition ID is required for raw copy.";
      return;
    }

    Version version;
    if (const auto parse_err = ParseVersion(render_version_, version);
        parse_err.has_value()) {
      render_status_text_ = "Error: " + *parse_err;
      return;
    }

    auto composition_result = Trim(render_version_).empty()
                                  ? engine_.LoadComposition(render_comp_id_)
                                  : engine_.LoadComposition(render_comp_id_, version);
    if (composition_result.HasError()) {
      render_status_text_ = "Error: " + composition_result.error().message;
      return;
    }

    auto raw_result = BuildRawRender(engine_, composition_result.value());
    if (raw_result.HasError()) {
      render_status_text_ = "Error: " + raw_result.error().message;
      return;
    }

    render_status_text_ = CopyToClipboardOsc52(raw_result.value())
                              ? "Copied raw composition render to terminal clipboard."
                              : "Failed to copy raw composition render.";
  });

  auto clear_button = ftxui::Button("Clear", [this] {
    render_params_.clear();
    render_output_ = "Enter composition ID and click Render";
    render_output_scroll_y_ = 0.f;
    render_status_text_ = "Render output actions are available after rendering.";
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
      {comp_id_input, version_input, params_input, render_button,
       copy_render_button, copy_raw_button, clear_button, refresh_button});
  auto controls_panel = ftxui::Renderer(
      controls,
      [comp_id_input, version_input, params_input, render_button, clear_button,
       refresh_button, copy_render_button, copy_raw_button, this] {
        const bool title_focused =
            comp_id_input->Focused() || version_input->Focused() ||
            params_input->Focused() || render_button->Focused() ||
            copy_render_button->Focused() || copy_raw_button->Focused() ||
            clear_button->Focused() || refresh_button->Focused();
        return ftxui::window(
                   ui::FocusedWindowTitle("Render Inputs", title_focused),
                   ftxui::vbox({
                       ftxui::text("Composition ID"),
                       comp_id_input->Render(),
                       ftxui::separator(),
                       ftxui::text("Version (optional)"),
                       version_input->Render(),
                       ftxui::text("Leave empty to render latest version") |
                           ftxui::dim,
                       ftxui::separator(),
                       ftxui::text("Runtime params"),
                       params_input->Render(),
                       ftxui::text("Format: key=value, key2=value2") |
                           ftxui::dim,
                       ftxui::separator(),
                       render_button->Render(),
                       copy_render_button->Render(),
                       copy_raw_button->Render(),
                       clear_button->Render(),
                       refresh_button->Render(),
                       ftxui::separator(),
                        ftxui::text(settings_strict_ ? "Strict mode: ON"
                                                    : "Strict mode: OFF") |
                           ftxui::dim,
                       ftxui::separator(),
                       ftxui::paragraph(render_status_text_) | ftxui::dim,
                   })) |
               ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 34) | ftxui::yflex;
      });
  auto list_panel = ftxui::Renderer(
      compositions_menu,
      [compositions_menu] {
        return ftxui::window(
                   ui::FocusedWindowTitle("Compositions",
                                          compositions_menu->Focused()),
                   compositions_menu->Render() | ftxui::flex) |
               ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 36) | ftxui::yflex;
      });

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

  return ftxui::Renderer(main, [output_component, list_panel, controls_panel] {
    return ftxui::hbox({
               list_panel->Render(),
               controls_panel->Render(),
               output_component->Render() | ftxui::xflex | ftxui::yflex,
           }) |
           ftxui::xflex | ftxui::yflex;
  });
}

}  // namespace tf
