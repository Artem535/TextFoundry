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
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "tf/obx_utils.hpp"

namespace tf {
namespace {
// ----------------------------------------------------------------------------
// Small UI helpers
// ----------------------------------------------------------------------------

ftxui::Element MakeHeader(std::string text,
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

inline void LTrim(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

inline void RTrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

inline void Trim(std::string& s) {
  LTrim(s);
  RTrim(s);
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
    Trim(key);
    Trim(val);

    if (!key.empty()) ctx.params[key] = val;

    pos = comma + 1;
  }
}

const std::vector<std::string>& BlockTypeLabels() {
  static const std::vector<std::string> labels = {"role", "constraint", "style",
                                                  "domain", "meta"};
  return labels;
}

bool IsBlockListEvent(const ftxui::Event& e) {
  return e == ftxui::Event::ArrowUp || e == ftxui::Event::ArrowDown ||
         e == ftxui::Event::Return || e.is_mouse();
}

ftxui::Element RenderCreateBlockForm(
    const std::string& modal_title, const std::string& submit_button_label,
    const ftxui::Component& block_id_input,
    const ftxui::Component& block_type_menu,
    const ftxui::Component& block_template_input,
    const ftxui::Component& block_defaults_input,
    const ftxui::Component& block_tags_input,
    const ftxui::Component& block_lang_input,
    const ftxui::Component& block_desc_input,
    const ftxui::Component& create_button, const ftxui::Component& reset_button,
    const ftxui::Component& cancel_button, const std::string& status_text) {
  return ftxui::window(
             ftxui::text(modal_title),
             ftxui::vbox({
                 ftxui::text("id") | ftxui::dim,
                 block_id_input->Render(),
                 ftxui::separator(),
                 ftxui::text("type") | ftxui::dim,
                 block_type_menu->Render() |
                     ftxui::size(ftxui::HEIGHT, ftxui::LESS_THAN, 5),
                 ftxui::separator(),
                 ftxui::text("template") | ftxui::dim,
                 block_template_input->Render(),
                 ftxui::separator(),
                 ftxui::text("defaults (key=value, comma separated)") |
                     ftxui::dim,
                 block_defaults_input->Render(),
                 ftxui::separator(),
                 ftxui::text("tags (comma separated)") | ftxui::dim,
                 block_tags_input->Render(),
                 ftxui::separator(),
                 ftxui::text("language") | ftxui::dim,
                 block_lang_input->Render(),
                 ftxui::separator(),
                 ftxui::text("description") | ftxui::dim,
                 block_desc_input->Render(),
                 ftxui::separator(),
                 ftxui::hbox({ftxui::text(submit_button_label + ": "),
                              create_button->Render(), ftxui::text(" "),
                              reset_button->Render(), ftxui::text(" "),
                              cancel_button->Render()}),
                 ftxui::separator(),
                 ftxui::paragraph(status_text) |
                     ftxui::color(ftxui::Color::LightSteelBlue),
             }) | ftxui::flex) |
         ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 72) |
         ftxui::size(ftxui::HEIGHT, ftxui::LESS_THAN, 34) | ftxui::center;
}
}  // namespace

// ============================================================================
// Constructor & Main Loop
// ============================================================================

Tui::Tui(Engine& engine) : engine_(engine), block_creator_(engine) {}

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

void Tui::RefreshBlocksList() {
  block_ids_ = engine_.ListBlocks();
  ClampIndex(selected_block_, static_cast<int>(block_ids_.size()));
}

void Tui::UpdateSelectedBlockDetails() {
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
    return;
  }

  details_text_ = "Error: " + result.error().message;
}

void Tui::SelectBlockById(const std::string& block_id) {
  if (block_ids_.empty()) return;

  auto it = std::find(block_ids_.begin(), block_ids_.end(), block_id);
  if (it == block_ids_.end()) return;
  selected_block_ = static_cast<int>(std::distance(block_ids_.begin(), it));
}

ftxui::Component Tui::BlocksTab() {
  RefreshBlocksList();
  UpdateSelectedBlockDetails();

  auto list = ftxui::Menu(&block_ids_, &selected_block_);
  auto open_create_modal_button = ftxui::Button("New Block", [this] {
    block_creator_.BeginCreate();
    show_create_block_modal_ = true;
  });
  auto open_edit_modal_button = ftxui::Button("Edit", [this] {
    if (block_ids_.empty()) {
      details_text_ = "No blocks found.";
      return;
    }
    auto result = engine_.LoadBlock(block_ids_[selected_block_]);
    if (result.HasError()) {
      details_text_ = "Error: " + result.error().message;
      return;
    }

    block_creator_.BeginEdit(result.value());
    show_create_block_modal_ = true;
  });

  auto block_id_input = ftxui::Input(&block_creator_.BlockId(), "role.system");
  auto block_type_menu =
      ftxui::Menu(&BlockTypeLabels(), &block_creator_.BlockTypeIndex());
  auto block_template_input =
      ftxui::Input(&block_creator_.BlockTemplate(), "Template with {{params}}");
  auto block_defaults_input = ftxui::Input(&block_creator_.BlockDefaults(),
                                           "name=value, max_tokens=1000");
  auto block_tags_input =
      ftxui::Input(&block_creator_.BlockTags(), "tag1, tag2");
  auto block_lang_input = ftxui::Input(&block_creator_.BlockLanguage(), "en");
  auto block_desc_input =
      ftxui::Input(&block_creator_.BlockDescription(), "Description");

  auto create_button = ftxui::Button("Save", [this] {
    auto created_block_id = block_creator_.Create();
    if (!created_block_id.has_value()) return;
    RefreshBlocksList();
    SelectBlockById(*created_block_id);
    UpdateSelectedBlockDetails();
    show_create_block_modal_ = false;
  });
  auto reset_button =
      ftxui::Button("Reset", [this] { block_creator_.Reset("Form reset"); });
  auto cancel_button =
      ftxui::Button("Cancel", [this] { show_create_block_modal_ = false; });

  auto create_buttons = ftxui::Container::Horizontal(
      {create_button, reset_button, cancel_button});
  auto create_form = ftxui::Container::Vertical(
      {block_id_input, block_type_menu, block_template_input,
       block_defaults_input, block_tags_input, block_lang_input,
       block_desc_input, create_buttons});

  auto details = ftxui::Renderer([this] {
    return ftxui::window(ftxui::text(" Details "),
                         ftxui::paragraph(details_text_) | ftxui::flex) |
           ftxui::flex;
  });

  auto create_panel = ftxui::Renderer(
      create_form,
      [&, block_id_input, block_type_menu, block_template_input,
       block_defaults_input, block_tags_input, block_lang_input,
       block_desc_input, create_button, reset_button, cancel_button] {
        return RenderCreateBlockForm(
            block_creator_.ModalTitle(), block_creator_.SubmitButtonLabel(),
            block_id_input, block_type_menu, block_template_input,
            block_defaults_input, block_tags_input, block_lang_input,
            block_desc_input, create_button, reset_button, cancel_button,
            block_creator_.Status());
      });

  auto content = ftxui::Container::Horizontal(
      {list, details, open_create_modal_button, open_edit_modal_button});
  content =
      content | ftxui::CatchEvent([this](const ftxui::Event& e) {
        if (IsBlockListEvent(e)) {
          ClampIndex(selected_block_, static_cast<int>(block_ids_.size()));
          UpdateSelectedBlockDetails();
        }
        return false;
      });

  auto body = ftxui::Renderer(content, [list, details, open_create_modal_button,
                                        open_edit_modal_button] {
    auto left =
        ftxui::window(ftxui::text(" Blocks "), list->Render() | ftxui::flex) |
        ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 36) | ftxui::flex;
    auto actions = ftxui::window(
        ftxui::text(" Actions "),
        ftxui::vbox({open_create_modal_button->Render(), ftxui::separator(),
                     open_edit_modal_button->Render()}) |
            ftxui::flex);

    return ftxui::hbox({
               left,
               details->Render() | ftxui::flex,
               actions | ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 24),
           }) |
           ftxui::flex;
  });

  auto with_modal = ftxui::Modal(body, create_panel, &show_create_block_modal_);
  return ftxui::CatchEvent(with_modal, [this](const ftxui::Event& e) {
    if (show_create_block_modal_ && e == ftxui::Event::Escape) {
      show_create_block_modal_ = false;
      return true;
    }
    return false;
  });
}

// ============================================================================
// Compositions Tab
// ============================================================================

ftxui::Component Tui::CompositionsTab() {
  auto root = ftxui::Container::Vertical({});
  return ftxui::Renderer(root, [] {
    return ftxui::vbox({
               MakeHeader("Compositions", ftxui::Color::Green),
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
               MakeHeader("Render", ftxui::Color::Magenta),
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
               MakeHeader("Settings", ftxui::Color::Yellow),
               ftxui::separator(),
               ftxui::text("Temporary stub tab"),
               ftxui::text("Diagnostics mode: component content disabled") |
                   ftxui::dim,
           }) |
           ftxui::flex | ftxui::border;
  });
}
}  // namespace tf
