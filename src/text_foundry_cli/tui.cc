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

    ftxui::Element make_header(std::string text,
                               ftxui::Color c = ftxui::Color::White) {
      return ftxui::text(std::move(text)) | ftxui::bold | ftxui::color(c) |
             ftxui::center;
    }

    // Consider "user input" as any non-mouse, non-cursor, non-custom event.
    // This includes arrows, enter, function keys, etc.
    bool IsUserInputEvent(const ftxui::Event &e) {
      if (e.is_mouse()) return false;
      if (e.is_cursor_position()) return false;
      if (e.is_cursor_shape()) return false;
      if (e == ftxui::Event::Custom) return false;
      return true;
    }

    // Exit shortcuts. F10 mapping can vary between terminals; Esc and 'q' are
    // reliable.
    bool IsExitEvent(const ftxui::Event &e) {
      if (e == ftxui::Event::F10) return true;
      if (e == ftxui::Event::Escape) return true;
      if (e == ftxui::Event::Character('q')) return true;
      // Common CSI sequence for F10 in many terminals.
      if (e.input() == "\x1b[21~") return true;
      return false;
    }

    void ClampIndex(int &idx, int size) {
      if (size <= 0) {
        idx = 0;
        return;
      }
      idx = std::max(0, std::min(idx, size - 1));
    }

    inline void ltrim(std::string &s) {
      s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
      }));
    }

    inline void rtrim(std::string &s) {
      s.erase(std::find_if(s.rbegin(), s.rend(),
                           [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
              s.end());
    }

    inline void trim(std::string &s) {
      ltrim(s);
      rtrim(s);
    }

    // Parse "key=value, key2=value2" into ctx.params.
    // Spaces around keys are trimmed; values are kept as-is (except surrounding
    // spaces).
    void ParseParamsInto(tf::RenderContext &ctx, const std::string &raw) {
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

    tf::Params ParseParamsRaw(const std::string &raw) {
      tf::Params params;
      std::size_t pos = 0;
      while (pos < raw.size()) {
        while (pos < raw.size() && (raw[pos] == ' ' || raw[pos] == ',')) pos++;
        if (pos >= raw.size()) break;

        const std::size_t eq = raw.find('=', pos);
        if (eq == std::string::npos) {
          throw std::invalid_argument("defaults format: key=value, key2=value2");
        }

        std::size_t comma = raw.find(',', eq + 1);
        if (comma == std::string::npos) comma = raw.size();

        std::string key = raw.substr(pos, eq - pos);
        std::string val = raw.substr(eq + 1, comma - (eq + 1));
        trim(key);
        trim(val);
        if (key.empty()) {
          throw std::invalid_argument("defaults key cannot be empty");
        }
        params[key] = val;
        pos = comma + 1;
      }
      return params;
    }

    std::vector<std::string> ParseTagsRaw(const std::string &raw) {
      std::vector<std::string> tags;
      std::size_t pos = 0;
      while (pos < raw.size()) {
        while (pos < raw.size() && (raw[pos] == ' ' || raw[pos] == ',')) pos++;
        if (pos >= raw.size()) break;

        std::size_t comma = raw.find(',', pos);
        if (comma == std::string::npos) comma = raw.size();

        std::string tag = raw.substr(pos, comma - pos);
        trim(tag);
        if (!tag.empty()) tags.push_back(std::move(tag));
        pos = comma + 1;
      }
      return tags;
    }

    const std::vector<std::string> &BlockTypeLabels() {
      static const std::vector<std::string> labels = {
        "role", "constraint", "style", "domain", "meta"
      };
      return labels;
    }

    const std::vector<BlockType> &BlockTypes() {
      static const std::vector<BlockType> values = {
        BlockType::Role, BlockType::Constraint, BlockType::Style,
        BlockType::Domain, BlockType::Meta
      };
      return values;
    }

    bool IsBlockListEvent(const ftxui::Event &e) {
      return e == ftxui::Event::ArrowUp || e == ftxui::Event::ArrowDown ||
             e == ftxui::Event::Return || e.is_mouse();
    }

    ftxui::Element RenderCreateBlockForm(
      const ftxui::Component &block_id_input,
      const ftxui::Component &block_type_menu,
      const ftxui::Component &block_template_input,
      const ftxui::Component &block_defaults_input,
      const ftxui::Component &block_tags_input,
      const ftxui::Component &block_lang_input,
      const ftxui::Component &block_desc_input,
      const ftxui::Component &create_button,
      const ftxui::Component &reset_button,
      const std::string &status_text) {
      return ftxui::window(
               ftxui::text(" Create Block "),
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
                 ftxui::hbox({
                   create_button->Render(), ftxui::text(" "),
                   reset_button->Render()
                 }),
                 ftxui::separator(),
                 ftxui::paragraph(status_text) |
                 ftxui::color(ftxui::Color::LightSteelBlue),
               }) |
               ftxui::flex) |
             ftxui::flex;
    }
  } // namespace

  // ============================================================================
  // Constructor & Main Loop
  // ============================================================================

  Tui::Tui(Engine &engine) : engine_(engine) {
  }

  void Tui::run() {
    // Fullscreen mode needs a real interactive terminal.
    // If you prefer not to enforce this, you can remove the checks.
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
      std::cerr << "Error: TUI requires an interactive terminal (TTY).\n"
          << "Run: tf tui (from a real terminal emulator)\n";
      return;
    }

    const char *term = std::getenv("TERM");
    if (term == nullptr || std::string_view(term) == "dumb") {
      std::cerr << "Error: TERM is not set (or set to 'dumb'). "
          << "Use a terminal with cursor support (e.g., xterm-256color).\n";
      return;
    }

    const char *force_terminal_output = std::getenv("TF_TUI_TERMINAL_OUTPUT");
    const bool use_terminal_output =
        force_terminal_output != nullptr &&
        std::string_view(force_terminal_output) == "1";

    // Fullscreen is the default mode.
    // TerminalOutput is available as explicit fallback.
    auto screen = use_terminal_output
                    ? ftxui::ScreenInteractive::TerminalOutput()
                    : ftxui::ScreenInteractive::Fullscreen();

    auto root = MainLayout(screen);
    // Force first repaint to avoid occasional "blank frame" on startup.
    screen.PostEvent(ftxui::Event::Custom);
    screen.Loop(root);
  }

  // ============================================================================
  // Main Layout
  // ============================================================================

  ftxui::Component Tui::MainLayout(auto &screen) {
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

    return ftxui::CatchEvent(with_modal, [&, tab_toggle](const ftxui::Event &e) {
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
      const auto &b = result.value();
      details_text_ = fmt::format("ID: {}\nVersion: {}.{}\nType: {}\nTemplate: {}",
                                  b.Id(), b.version().major, b.version().minor,
                                  BlockTypeToString(b.type()), b.templ().Сontent());
      return;
    }

    details_text_ = "Error: " + result.error().message;
  }

  void Tui::ResetCreateBlockForm(const std::string &status) {
    create_block_id_.clear();
    create_template_.clear();
    create_defaults_.clear();
    create_tags_.clear();
    create_description_.clear();
    create_language_ = "en";
    create_block_type_ = 3;
    create_status_ = status;
  }

  void Tui::SelectBlockById(const std::string &block_id) {
    if (block_ids_.empty()) return;

    auto it = std::find(block_ids_.begin(), block_ids_.end(), block_id);
    if (it == block_ids_.end()) return;
    selected_block_ = static_cast<int>(std::distance(block_ids_.begin(), it));
  }

  void Tui::TryCreateBlock() {
    try {
      std::string block_id = create_block_id_;
      std::string block_template = create_template_;
      std::string block_lang = create_language_;
      trim(block_id);
      trim(block_template);
      trim(block_lang);

      if (block_id.empty()) {
        create_status_ = "Error: block id is required";
        return;
      }
      if (block_template.empty()) {
        create_status_ = "Error: template is required";
        return;
      }

      const auto &block_types = BlockTypes();
      if (create_block_type_ < 0 ||
          create_block_type_ >= static_cast<int>(block_types.size())) {
        create_status_ = "Error: invalid block type";
        return;
      }

      if (block_lang.empty()) block_lang = "en";

      auto builder = BlockDraftBuilder(block_id)
                         .WithTemplate(Template(block_template))
                         .WithType(block_types[create_block_type_])
                         .WithLanguage(block_lang);

      auto defaults = ParseParamsRaw(create_defaults_);
      if (!defaults.empty()) {
        builder.WithDefaults(std::move(defaults));
      }

      for (const auto &tag: ParseTagsRaw(create_tags_)) {
        builder.WithTag(tag);
      }

      if (!create_description_.empty()) {
        builder.WithDescription(create_description_);
      }

      auto draft = builder.build();
      auto result = engine_.PublishBlock(std::move(draft), Engine::VersionBump::Minor);
      if (result.HasError()) {
        create_status_ = "Error: " + result.error().message;
        return;
      }

      const auto &pub = result.value();
      create_status_ = fmt::format("Created: {}@{}", pub.id(), pub.version().ToString());

      RefreshBlocksList();
      SelectBlockById(pub.id());
      UpdateSelectedBlockDetails();

      create_block_id_.clear();
      create_template_.clear();
      create_defaults_.clear();
      create_tags_.clear();
      create_description_.clear();
    } catch (const std::exception &ex) {
      create_status_ = "Error: " + std::string(ex.what());
    }
  }

  ftxui::Component Tui::BlocksTab() {
    RefreshBlocksList();
    UpdateSelectedBlockDetails();

    auto list = ftxui::Menu(&block_ids_, &selected_block_);

    auto block_id_input = ftxui::Input(&create_block_id_, "role.system");
    auto block_type_menu = ftxui::Menu(&BlockTypeLabels(), &create_block_type_);
    auto block_template_input =
        ftxui::Input(&create_template_, "Template with {{params}}");
    auto block_defaults_input =
        ftxui::Input(&create_defaults_, "name=value, max_tokens=1000");
    auto block_tags_input = ftxui::Input(&create_tags_, "tag1, tag2");
    auto block_lang_input = ftxui::Input(&create_language_, "en");
    auto block_desc_input = ftxui::Input(&create_description_, "Description");

    auto create_button = ftxui::Button("Create", [this] { TryCreateBlock(); });
    auto reset_button =
        ftxui::Button("Reset", [this] { ResetCreateBlockForm("Form reset"); });

    auto create_buttons =
        ftxui::Container::Horizontal({create_button, reset_button});
    auto create_form = ftxui::Container::Vertical(
      {
        block_id_input, block_type_menu, block_template_input,
        block_defaults_input, block_tags_input, block_lang_input,
        block_desc_input, create_buttons
      });

    auto details = ftxui::Renderer([this] {
      return ftxui::window(ftxui::text(" Details "),
                           ftxui::paragraph(details_text_) | ftxui::flex) |
             ftxui::flex;
    });

    auto create_panel = ftxui::Renderer(
        create_form,
        [&, block_id_input, block_type_menu, block_template_input,
         block_defaults_input, block_tags_input, block_lang_input,
         block_desc_input, create_button, reset_button] {
          return RenderCreateBlockForm(
              block_id_input, block_type_menu, block_template_input,
              block_defaults_input, block_tags_input, block_lang_input,
              block_desc_input, create_button, reset_button, create_status_);
        });

    auto content = ftxui::Container::Horizontal({list, details, create_panel});
    content =
        content |
        ftxui::CatchEvent([this](const ftxui::Event &e) {
          if (IsBlockListEvent(e)) {
            ClampIndex(selected_block_, static_cast<int>(block_ids_.size()));
            UpdateSelectedBlockDetails();
          }
          return false;
        });

    return ftxui::Renderer(content, [list, details, create_panel] {
      auto left =
          ftxui::window(ftxui::text(" Blocks "), list->Render() | ftxui::flex) |
          ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 36) | ftxui::flex;

      return ftxui::hbox({
               left,
               details->Render() | ftxui::flex,
               create_panel->Render() | ftxui::flex,
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
} // namespace tf
