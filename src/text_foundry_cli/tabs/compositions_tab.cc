#include "../tui.h"
#include "tab_header.h"

#include <algorithm>
#include <cctype>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>
#include <optional>
#include <sstream>
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

std::string ToLower(std::string s) {
  std::ranges::transform(
      s, s.begin(),
      [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

std::optional<std::string> ParseVersion(const std::string& input,
                                        Version& out) {
  const auto dot = input.find('.');
  if (dot == std::string::npos) {
    return "Version must be major.minor";
  }
  const auto major_str = Trim(input.substr(0, dot));
  const auto minor_str = Trim(input.substr(dot + 1));
  if (major_str.empty() || minor_str.empty()) {
    return "Version must be major.minor";
  }
  try {
    const auto major = std::stoi(major_str);
    const auto minor = std::stoi(minor_str);
    if (major < 0 || major > 65535 || minor < 0 || minor > 65535) {
      return "Version values must be in 0..65535";
    }
    out = Version{static_cast<uint16_t>(major), static_cast<uint16_t>(minor)};
    return std::nullopt;
  } catch (const std::exception&) {
    return "Version must be numeric";
  }
}

std::optional<std::string> AddBlockRefFromLine(CompositionDraftBuilder& builder,
                                               const std::string& line) {
  const auto at = line.find('@');
  if (at == std::string::npos) {
    return "Block ref must include version: " + line;
  }

  const std::string block_id = Trim(line.substr(0, at));
  std::string rest = Trim(line.substr(at + 1));
  if (block_id.empty() || rest.empty()) {
    return "Invalid block ref: " + line;
  }

  std::string version_text = rest;
  std::string params_text;
  const auto q = rest.find('?');
  if (q != std::string::npos) {
    version_text = Trim(rest.substr(0, q));
    params_text = Trim(rest.substr(q + 1));
  }

  Version version;
  if (const auto err = ParseVersion(version_text, version); err.has_value()) {
    return "Invalid block ref version in '" + line + "': " + *err;
  }

  Params params;
  if (!params_text.empty()) {
    std::size_t pos = 0;
    while (pos <= params_text.size()) {
      const auto amp = params_text.find('&', pos);
      const auto token = params_text.substr(
          pos, amp == std::string::npos ? std::string::npos : amp - pos);
      const auto kv = Trim(token);
      if (!kv.empty()) {
        const auto eq = kv.find('=');
        if (eq == std::string::npos) {
          return "Invalid local param format in '" + line + "'";
        }
        const auto key = Trim(kv.substr(0, eq));
        const auto value = Trim(kv.substr(eq + 1));
        if (key.empty()) {
          return "Empty local param key in '" + line + "'";
        }
        params[key] = value;
      }
      if (amp == std::string::npos) break;
      pos = amp + 1;
    }
  }

  builder.AddBlockRef(block_id, version.major, version.minor, params);
  return std::nullopt;
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
  out << "id: " << comp.id() << "\n";
  out << "version: " << comp.version().ToString() << "\n";
  out << "state: " << BlockStateToString(comp.state()) << "\n";
  out << "fragments: " << comp.fragmentCount() << "\n";
  if (!comp.description().empty()) {
    out << "description: " << comp.description() << "\n";
  }
  out << "\ncontent:\n";

  size_t idx = 0;
  for (const auto& fragment : comp.fragments()) {
    out << "  " << idx++ << ". ";
    if (fragment.IsBlockRef()) {
      const auto& ref = fragment.AsBlockRef();
      out << "block " << ref.GetBlockId();
      if (ref.version().has_value()) {
        out << "@" << ref.version()->ToString();
      }
      if (!ref.LocalParams().empty()) {
        out << " [" << ref.LocalParams().size() << " params]";
      }
      out << "\n";
      continue;
    }
    if (fragment.IsStaticText()) {
      auto text = fragment.AsStaticText().text();
      std::ranges::replace(text, '\n', ' ');
      if (text.size() > 96) text = text.substr(0, 93) + "...";
      out << "text: \"" << text << "\"\n";
      continue;
    }
    out << "separator: " << SeparatorTypeToString(fragment.AsSeparator().type)
        << "\n";
  }

  comp_details_text_ = out.str();
}

ftxui::Component Tui::CompositionsTab() {
  RefreshCompositionsList();
  UpdateSelectedCompositionDetails();

  auto list = ftxui::Menu(&comp_ids_, &selected_comp_);

  auto open_create_modal_button = ftxui::Button("New Composition", [this] {
    comp_edit_mode_ = false;
    comp_create_id_.clear();
    comp_create_description_.clear();
    comp_create_fragments_preview_.clear();
    comp_create_fragment_specs_.clear();
    selected_comp_fragment_ = 0;
    comp_create_status_ = "Build composition and press Create";

    comp_add_block_id_.clear();
    comp_add_block_version_.clear();
    comp_add_block_params_.clear();
    comp_add_static_text_.clear();
    comp_add_block_status_ = "Enter block_ref fields and submit";
    comp_add_text_status_ = "Enter static text and submit";

    show_create_comp_modal_ = true;
    show_add_block_ref_modal_ = false;
    show_add_static_text_modal_ = false;
    focus_comp_modal_on_open_ = true;
  });
  auto open_edit_modal_button = ftxui::Button("Edit Composition", [this] {
    if (comp_ids_.empty()) {
      comp_details_text_ = "No composition selected.";
      return;
    }

    auto result = engine_.LoadComposition(comp_ids_[selected_comp_]);
    if (result.HasError()) {
      comp_details_text_ = "Error: " + result.error().message;
      return;
    }

    comp_edit_mode_ = true;
    const auto& comp = result.value();
    comp_create_id_ = comp.id();
    comp_create_description_ = comp.description();
    comp_create_fragments_preview_.clear();
    comp_create_fragment_specs_.clear();
    selected_comp_fragment_ = 0;

    for (const auto& fragment : comp.fragments()) {
      if (fragment.IsBlockRef()) {
        const auto& ref = fragment.AsBlockRef();
        if (!ref.version().has_value()) {
          continue;
        }
        std::string line = ref.GetBlockId() + "@" + ref.version()->ToString();
        if (!ref.LocalParams().empty()) {
          bool first = true;
          line += "?";
          for (const auto& [key, value] : ref.LocalParams()) {
            if (!first) line += "&";
            line += key + "=" + value;
            first = false;
          }
        }
        comp_create_fragment_specs_.push_back("B|" + line);

        std::string preview =
            "BlockRef " + ref.GetBlockId() + "@" + ref.version()->ToString();
        if (!ref.LocalParams().empty()) {
          preview += " [" + std::to_string(ref.LocalParams().size()) +
                     " params]";
        }
        comp_create_fragments_preview_.push_back(preview);
        continue;
      }

      if (fragment.IsStaticText()) {
        const auto text = fragment.AsStaticText().text();
        comp_create_fragment_specs_.push_back("T|" + text);
        auto preview = text;
        std::ranges::replace(preview, '\n', ' ');
        if (preview.size() > 80) preview = preview.substr(0, 77) + "...";
        comp_create_fragments_preview_.push_back("StaticText \"" + preview + "\"");
        continue;
      }

      const auto type = fragment.AsSeparator().type;
      if (type == SeparatorType::Newline) {
        comp_create_fragment_specs_.push_back("S|newline");
        comp_create_fragments_preview_.push_back("Separator \\n");
      } else if (type == SeparatorType::Paragraph) {
        comp_create_fragment_specs_.push_back("S|paragraph");
        comp_create_fragments_preview_.push_back("Separator \\n\\n");
      } else {
        comp_create_fragment_specs_.push_back("S|hr");
        comp_create_fragments_preview_.push_back("Separator ---");
      }
    }

    if (!comp_create_fragments_preview_.empty()) {
      selected_comp_fragment_ =
          static_cast<int>(comp_create_fragments_preview_.size()) - 1;
    }
    comp_create_status_ = "Edit mode: update fragments and press Save";
    comp_add_block_id_.clear();
    comp_add_block_version_.clear();
    comp_add_block_params_.clear();
    comp_add_static_text_.clear();
    comp_add_block_status_ = "Enter block_ref fields and submit";
    comp_add_text_status_ = "Enter static text and submit";

    show_create_comp_modal_ = true;
    show_add_block_ref_modal_ = false;
    show_add_static_text_modal_ = false;
    focus_comp_modal_on_open_ = true;
  });
  auto refresh_button = ftxui::Button("Refresh", [this] {
    RefreshCompositionsList();
    UpdateSelectedCompositionDetails();
  });

  auto details = ftxui::Renderer([this] {
    return ftxui::window(
               ftxui::text(" Details "),
               ftxui::paragraph(comp_details_text_) | ftxui::vscroll_indicator |
                   ftxui::yframe | ftxui::xflex | ftxui::yflex) |
           ftxui::xflex | ftxui::yflex;
  });

  auto list_and_actions =
      ftxui::Container::Vertical(
          {list, open_create_modal_button, open_edit_modal_button, refresh_button});
  auto main_content = ftxui::Container::Horizontal({list_and_actions, details});
  main_content = main_content | ftxui::CatchEvent([this](const ftxui::Event& e) {
                   if (show_create_comp_modal_) return false;
                   if (IsCompositionListEvent(e)) {
                     ClampIndex(selected_comp_, static_cast<int>(comp_ids_.size()));
                     UpdateSelectedCompositionDetails();
                   }
                   return false;
                 });

  auto body = ftxui::Renderer(
      main_content,
      [list, details, open_create_modal_button, open_edit_modal_button,
       refresh_button] {
        auto left = ftxui::window(
                        ftxui::text(" List "),
                        ftxui::vbox({
                            list->Render() | ftxui::vscroll_indicator |
                                ftxui::yframe | ftxui::xflex | ftxui::yflex,
                            ftxui::separator(),
                            open_create_modal_button->Render(),
                            open_edit_modal_button->Render(),
                            refresh_button->Render(),
                        }) |
                            ftxui::xflex | ftxui::yflex) |
                    ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 42) | ftxui::yflex;

        return ftxui::hbox({
                   left,
                   details->Render() | ftxui::xflex | ftxui::yflex,
               }) |
               ftxui::xflex | ftxui::yflex;
      });

  auto desc_options = ftxui::InputOption::Default();
  desc_options.multiline = true;
  auto static_text_options = ftxui::InputOption::Default();
  static_text_options.multiline = true;

  auto comp_id_input = ftxui::Input(&comp_create_id_, "welcome.message");
  auto description_input =
      ftxui::Input(&comp_create_description_, "Composition purpose", desc_options);
  auto fragments_menu =
      ftxui::Menu(&comp_create_fragments_preview_, &selected_comp_fragment_);

  auto add_block_ref_button = ftxui::Button("Add BlockRef", [this] {
    comp_add_block_id_.clear();
    comp_add_block_version_.clear();
    comp_add_block_params_.clear();
    comp_block_picker_search_.clear();
    comp_block_picker_ids_ = engine_.ListBlocks();
    comp_block_picker_filtered_ids_ = comp_block_picker_ids_;
    selected_comp_block_picker_ = 0;
    comp_add_block_status_ = "Enter block_ref fields and submit";
    show_add_block_ref_modal_ = true;
  });
  auto add_static_text_button = ftxui::Button("Add StaticText", [this] {
    comp_add_static_text_.clear();
    comp_add_text_status_ = "Enter static text and submit";
    show_add_static_text_modal_ = true;
  });
  auto insert_newlines_button = ftxui::Button("Insert \\n Between", [this] {
    if (comp_create_fragment_specs_.size() < 2) {
      comp_create_status_ = "Need at least 2 fragments";
      return;
    }

    std::vector<std::string> updated_specs;
    std::vector<std::string> updated_preview;
    updated_specs.reserve(comp_create_fragment_specs_.size() * 2);
    updated_preview.reserve(comp_create_fragments_preview_.size() * 2);

    for (size_t i = 0; i < comp_create_fragment_specs_.size(); ++i) {
      updated_specs.push_back(comp_create_fragment_specs_[i]);
      updated_preview.push_back(comp_create_fragments_preview_[i]);
      if (i + 1 >= comp_create_fragment_specs_.size()) {
        continue;
      }
      const bool left_is_separator = comp_create_fragment_specs_[i].starts_with("S|");
      const bool right_is_separator =
          comp_create_fragment_specs_[i + 1].starts_with("S|");
      if (!left_is_separator && !right_is_separator) {
        updated_specs.push_back("S|newline");
        updated_preview.push_back("Separator \\n");
      }
    }

    comp_create_fragment_specs_ = std::move(updated_specs);
    comp_create_fragments_preview_ = std::move(updated_preview);
    ClampIndex(selected_comp_fragment_,
               static_cast<int>(comp_create_fragments_preview_.size()));
    comp_create_status_ = "Inserted \\n separators between fragments";
  });
  auto remove_fragment_button = ftxui::Button("Remove Selected", [this] {
    if (comp_create_fragment_specs_.empty()) {
      comp_create_status_ = "Nothing to remove";
      return;
    }
    const int idx = std::max(
        0, std::min(selected_comp_fragment_,
                    static_cast<int>(comp_create_fragment_specs_.size()) - 1));
    comp_create_fragment_specs_.erase(comp_create_fragment_specs_.begin() + idx);
    comp_create_fragments_preview_.erase(comp_create_fragments_preview_.begin() +
                                         idx);
    ClampIndex(selected_comp_fragment_,
               static_cast<int>(comp_create_fragments_preview_.size()));
    comp_create_status_ = "Removed fragment";
  });

  auto create_button = ftxui::Button("Create", [this] {
    auto comp_id = Trim(comp_create_id_);
    if (comp_id.empty()) {
      comp_create_status_ = "Error: composition id is required";
      return;
    }
    if (comp_create_fragment_specs_.empty()) {
      comp_create_status_ = "Error: add at least one fragment";
      return;
    }

    CompositionDraftBuilder builder(comp_id);
    if (!Trim(comp_create_description_).empty()) {
      builder.WithDescription(Trim(comp_create_description_));
    }
    const bool has_separator_specs = std::ranges::any_of(
        comp_create_fragment_specs_,
        [](const std::string& spec) { return spec.starts_with("S|"); });
    if (settings_comp_newline_delimiter_ && !has_separator_specs) {
      auto style = StyleProfile::plain();
      style.structural.delimiter = "\n";
      builder.WithStyleProfile(std::move(style));
    }

    for (const auto& spec : comp_create_fragment_specs_) {
      if (spec.starts_with("B|")) {
        if (const auto err = AddBlockRefFromLine(builder, spec.substr(2));
            err.has_value()) {
          comp_create_status_ = "Error: " + *err;
          return;
        }
        continue;
      }
      if (spec.starts_with("T|")) {
        builder.AddStaticText(spec.substr(2));
        continue;
      }
      if (spec.starts_with("S|")) {
        const auto sep = Trim(spec.substr(2));
        if (sep == "newline") {
          builder.AddSeparator(SeparatorType::Newline);
          continue;
        }
        if (sep == "paragraph") {
          builder.AddSeparator(SeparatorType::Paragraph);
          continue;
        }
        if (sep == "hr") {
          builder.AddSeparator(SeparatorType::Hr);
          continue;
        }
        comp_create_status_ = "Error: unknown separator type";
        return;
      }
      comp_create_status_ = "Error: unknown fragment spec";
      return;
    }

    auto result =
        engine_.PublishComposition(builder.build(), Engine::VersionBump::Minor);
    if (result.HasError()) {
      comp_create_status_ = "Error: " + result.error().message;
      return;
    }

    const auto created_id = result.value().id();
    comp_create_status_ =
        (comp_edit_mode_ ? "Updated: " : "Created: ") + created_id + "@" +
        result.value().version().ToString();
    comp_edit_mode_ = false;
    show_create_comp_modal_ = false;
    show_add_block_ref_modal_ = false;
    show_add_static_text_modal_ = false;
    RefreshCompositionsList();
    const auto it = std::ranges::find(comp_ids_, created_id);
    if (it != comp_ids_.end()) {
      selected_comp_ = static_cast<int>(std::distance(comp_ids_.begin(), it));
    }
    UpdateSelectedCompositionDetails();
  });

  auto cancel_button = ftxui::Button("Cancel", [this] {
    comp_edit_mode_ = false;
    show_create_comp_modal_ = false;
    show_add_block_ref_modal_ = false;
    show_add_static_text_modal_ = false;
    focus_comp_modal_on_open_ = false;
  });

  auto apply_block_picker_filter = [this] {
    const auto search = ToLower(Trim(comp_block_picker_search_));
    comp_block_picker_filtered_ids_.clear();
    for (const auto& id : comp_block_picker_ids_) {
      if (search.empty() || ToLower(id).contains(search)) {
        comp_block_picker_filtered_ids_.push_back(id);
      }
    }
    ClampIndex(selected_comp_block_picker_,
               static_cast<int>(comp_block_picker_filtered_ids_.size()));
  };

  auto block_picker_search_options = ftxui::InputOption::Default();
  block_picker_search_options.on_change = [apply_block_picker_filter] {
    apply_block_picker_filter();
  };
  auto block_picker_search_input =
      ftxui::Input(&comp_block_picker_search_, "search block id",
                   block_picker_search_options);
  auto block_picker_menu = ftxui::Menu(&comp_block_picker_filtered_ids_,
                                       &selected_comp_block_picker_);
  auto add_block_version_input = ftxui::Input(&comp_add_block_version_, "1.0");
  auto add_block_params_input =
      ftxui::Input(&comp_add_block_params_, "name=Alice&lang=en");

  auto add_block_submit_button = ftxui::Button("Submit", [this] {
    if (comp_block_picker_filtered_ids_.empty()) {
      comp_add_block_status_ = "Error: no block selected";
      return;
    }
    const auto idx =
        std::max(0, std::min(selected_comp_block_picker_,
                             static_cast<int>(comp_block_picker_filtered_ids_.size()) -
                                 1));
    const auto block_id = Trim(comp_block_picker_filtered_ids_[idx]);
    const auto version = Trim(comp_add_block_version_);
    auto params = Trim(comp_add_block_params_);
    std::ranges::replace(params, ',', '&');

    if (block_id.empty()) {
      comp_add_block_status_ = "Error: block id is required";
      return;
    }
    if (version.empty()) {
      comp_add_block_status_ = "Error: version is required";
      return;
    }

    std::string line = block_id + "@" + version;
    if (!params.empty()) {
      line += "?" + params;
    }

    CompositionDraftBuilder dummy("tmp");
    if (const auto err = AddBlockRefFromLine(dummy, line); err.has_value()) {
      comp_add_block_status_ = "Error: " + *err;
      return;
    }

    comp_create_fragment_specs_.push_back("B|" + line);
    std::string preview = "BlockRef " + block_id + "@" + version;
    if (!params.empty()) preview += " [" + params + "]";
    comp_create_fragments_preview_.push_back(preview);
    selected_comp_fragment_ =
        static_cast<int>(comp_create_fragments_preview_.size()) - 1;
    comp_create_status_ = "Added BlockRef";
    show_add_block_ref_modal_ = false;
  });

  auto add_block_cancel_button =
      ftxui::Button("Cancel", [this] { show_add_block_ref_modal_ = false; });

  auto add_static_text_input =
      ftxui::Input(&comp_add_static_text_, "Any text fragment", static_text_options);
  auto add_static_submit_button = ftxui::Button("Submit", [this] {
    const auto text = Trim(comp_add_static_text_);
    if (text.empty()) {
      comp_add_text_status_ = "Error: static text is required";
      return;
    }

    comp_create_fragment_specs_.push_back("T|" + text);
    auto preview = text;
    std::ranges::replace(preview, '\n', ' ');
    if (preview.size() > 80) preview = preview.substr(0, 77) + "...";
    comp_create_fragments_preview_.push_back("StaticText \"" + preview + "\"");
    selected_comp_fragment_ =
        static_cast<int>(comp_create_fragments_preview_.size()) - 1;
    comp_create_status_ = "Added StaticText";
    show_add_static_text_modal_ = false;
  });

  auto add_static_cancel_button =
      ftxui::Button("Cancel", [this] { show_add_static_text_modal_ = false; });

  auto modal_form = ftxui::Container::Vertical(
      {comp_id_input, description_input, add_block_ref_button,
       add_static_text_button, insert_newlines_button, remove_fragment_button,
       fragments_menu, create_button, cancel_button});
  auto modal_panel = ftxui::Renderer(
      modal_form,
      [this, comp_id_input, description_input, fragments_menu,
       add_block_ref_button, add_static_text_button, insert_newlines_button,
       remove_fragment_button, create_button, cancel_button] {
        const auto terminal_size = ftxui::Terminal::Size();
        const int modal_width = std::max(80, terminal_size.dimx * 9 / 10);
        const int modal_height = std::max(24, terminal_size.dimy * 9 / 10);

        auto left = ftxui::window(
            ftxui::text(" Builder "),
            ftxui::vbox({
                ftxui::text("id") | ftxui::bold,
                comp_id_input->Render(),
                ftxui::separator(),
                ftxui::text("description") | ftxui::bold,
                description_input->Render(),
                ftxui::separator(),
                add_block_ref_button->Render(),
                add_static_text_button->Render(),
                insert_newlines_button->Render(),
                remove_fragment_button->Render(),
                ftxui::separator(),
                ftxui::hbox({
                    ftxui::text(comp_edit_mode_ ? "Mode: Edit" : "Mode: Create") |
                        ftxui::dim,
                }),
                create_button->Render(),
                cancel_button->Render(),
                ftxui::separator(),
                ftxui::paragraph(comp_create_status_) | ftxui::dim,
            }) |
                ftxui::xflex | ftxui::yflex);

        auto list_view = comp_create_fragments_preview_.empty()
                             ? (ftxui::text("No fragments yet") | ftxui::dim)
                             : (fragments_menu->Render() | ftxui::vscroll_indicator |
                                ftxui::frame | ftxui::xflex | ftxui::yflex);
        auto right = ftxui::window(ftxui::text(" Result List "),
                                   list_view | ftxui::xflex | ftxui::yflex);

        return ftxui::hbox({
                   left | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 52) |
                       ftxui::yflex,
                   right | ftxui::xflex | ftxui::yflex,
               }) |
               ftxui::xflex | ftxui::yflex |
               ftxui::borderStyled(ftxui::ROUNDED, ftxui::Color::Orange1) |
               ftxui::size(ftxui::WIDTH, ftxui::EQUAL, modal_width) |
               ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, modal_height) |
               ftxui::center;
      });

  modal_panel =
      modal_panel | ftxui::CatchEvent(
                        [this, comp_id_input, description_input,
                         add_block_ref_button, add_static_text_button,
                         insert_newlines_button, remove_fragment_button,
                         fragments_menu, create_button, cancel_button,
                         modal_focus_chain = std::vector<ftxui::Component>{
                             comp_id_input,
                             description_input,
                             add_block_ref_button,
                             add_static_text_button,
                             insert_newlines_button,
                             remove_fragment_button,
                             fragments_menu,
                             create_button,
                             cancel_button,
                         },
                         modal_focus_index = 0](const ftxui::Event& e) mutable {
                          if (!show_create_comp_modal_ || show_add_block_ref_modal_ ||
                              show_add_static_text_modal_) {
                            return false;
                          }

                          if (focus_comp_modal_on_open_) {
                            modal_focus_index = 0;
                            modal_focus_chain[modal_focus_index]->TakeFocus();
                            focus_comp_modal_on_open_ = false;
                          }

                          if (e != ftxui::Event::Tab &&
                              e != ftxui::Event::TabReverse) {
                            return false;
                          }

                          for (size_t i = 0; i < modal_focus_chain.size(); ++i) {
                            if (modal_focus_chain[i]->Focused()) {
                              modal_focus_index = static_cast<int>(i);
                              break;
                            }
                          }
                          if (e == ftxui::Event::Tab) {
                            modal_focus_index =
                                (modal_focus_index + 1) %
                                static_cast<int>(modal_focus_chain.size());
                          } else {
                            modal_focus_index =
                                (modal_focus_index - 1 +
                                 static_cast<int>(modal_focus_chain.size())) %
                                static_cast<int>(modal_focus_chain.size());
                          }
                          modal_focus_chain[modal_focus_index]->TakeFocus();
                          return true;
                        });

  auto add_block_form = ftxui::Container::Vertical(
      {block_picker_search_input, block_picker_menu, add_block_version_input,
       add_block_params_input,
       add_block_submit_button, add_block_cancel_button});
  auto add_block_modal =
      ftxui::Renderer(add_block_form,
                      [this, block_picker_search_input, block_picker_menu,
                       add_block_version_input,
                       add_block_params_input, add_block_submit_button,
                       add_block_cancel_button] {
                        auto picker_list = comp_block_picker_filtered_ids_.empty()
                                               ? (ftxui::text("No blocks found") |
                                                  ftxui::dim)
                                               : (block_picker_menu->Render() |
                                                  ftxui::vscroll_indicator |
                                                  ftxui::frame | ftxui::xflex |
                                                  ftxui::yflex);
                        const auto selected_block =
                            comp_block_picker_filtered_ids_.empty()
                                ? std::string("-")
                                : comp_block_picker_filtered_ids_[std::max(
                                      0, std::min(
                                             selected_comp_block_picker_,
                                             static_cast<int>(
                                                 comp_block_picker_filtered_ids_
                                                     .size()) -
                                                 1))];
                        auto content = ftxui::vbox({
                            ftxui::text(" Add BlockRef ") | ftxui::bold |
                                ftxui::center,
                            ftxui::separator(),
                            ftxui::text("search block") | ftxui::bold,
                            block_picker_search_input->Render(),
                            ftxui::separator(),
                            ftxui::text("block list") | ftxui::bold,
                            picker_list | ftxui::size(ftxui::HEIGHT,
                                                      ftxui::EQUAL, 5),
                            ftxui::separator(),
                            ftxui::text("selected: " + selected_block) |
                                ftxui::dim,
                            ftxui::separator(),
                            ftxui::text("version") | ftxui::bold,
                            add_block_version_input->Render(),
                            ftxui::separator(),
                            ftxui::text("local params (optional)") |
                                ftxui::bold,
                            add_block_params_input->Render(),
                            ftxui::separator(),
                            ftxui::hbox({add_block_submit_button->Render(),
                                         ftxui::text(" "),
                                         add_block_cancel_button->Render()}),
                            ftxui::separator(),
                            ftxui::paragraph(comp_add_block_status_) |
                                ftxui::dim,
                        });

                        return (content | ftxui::vscroll_indicator | ftxui::frame |
                                ftxui::xflex | ftxui::yflex) |
                               ftxui::borderStyled(ftxui::ROUNDED,
                                                   ftxui::Color::Green) |
                               ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 68) |
                               ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 24) |
                               ftxui::center;
                      });
  add_block_modal = add_block_modal | ftxui::CatchEvent(
                                        [this, block_picker_menu,
                                         add_block_version_input](
                                            const ftxui::Event& e) {
                                          if (!show_add_block_ref_modal_) {
                                            return false;
                                          }
                                          if (IsCompositionListEvent(e)) {
                                            ClampIndex(
                                                selected_comp_block_picker_,
                                                static_cast<int>(
                                                    comp_block_picker_filtered_ids_
                                                        .size()));
                                          }
                                          if (e == ftxui::Event::Return &&
                                              block_picker_menu->Focused()) {
                                            add_block_version_input->TakeFocus();
                                            return true;
                                          }
                                          return false;
                                        });

  auto add_text_form = ftxui::Container::Vertical(
      {add_static_text_input, add_static_submit_button, add_static_cancel_button});
  auto add_text_modal =
      ftxui::Renderer(add_text_form,
                      [this, add_static_text_input, add_static_submit_button,
                       add_static_cancel_button] {
                        auto content = ftxui::vbox({
                            ftxui::text(" Add Static Text ") | ftxui::bold |
                                ftxui::center,
                            ftxui::separator(),
                            ftxui::text("text") | ftxui::bold,
                            (add_static_text_input->Render() |
                             ftxui::focusCursorBar | ftxui::vscroll_indicator |
                             ftxui::frame | ftxui::xflex | ftxui::yflex),
                            ftxui::separator(),
                            ftxui::hbox({add_static_submit_button->Render(),
                                         ftxui::text(" "),
                                         add_static_cancel_button->Render()}),
                            ftxui::separator(),
                            ftxui::paragraph(comp_add_text_status_) | ftxui::dim,
                        });
                        return (content | ftxui::vscroll_indicator | ftxui::frame |
                                ftxui::xflex | ftxui::yflex) |
                               ftxui::borderStyled(ftxui::ROUNDED,
                                                   ftxui::Color::BlueLight) |
                               ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 78) |
                               ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 22) |
                               ftxui::center;
                      });

  auto with_modal = ftxui::Modal(body, modal_panel, &show_create_comp_modal_);
  auto with_block_modal =
      ftxui::Modal(with_modal, add_block_modal, &show_add_block_ref_modal_);
  auto with_text_modal = ftxui::Modal(with_block_modal, add_text_modal,
                                      &show_add_static_text_modal_);
  return ftxui::CatchEvent(with_text_modal, [this](const ftxui::Event& e) {
    if (show_add_static_text_modal_ && e == ftxui::Event::Escape) {
      show_add_static_text_modal_ = false;
      return true;
    }
    if (show_add_block_ref_modal_ && e == ftxui::Event::Escape) {
      show_add_block_ref_modal_ = false;
      return true;
    }
    if (show_create_comp_modal_ && e == ftxui::Event::Escape) {
      show_create_comp_modal_ = false;
      show_add_block_ref_modal_ = false;
      show_add_static_text_modal_ = false;
      focus_comp_modal_on_open_ = false;
      return true;
    }
    return false;
  });
}

}  // namespace tf
