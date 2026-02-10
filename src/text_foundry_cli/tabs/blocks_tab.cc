#include "../tui.h"
#include "blocks_modal_view.h"

#include <fmt/format.h>

#include <algorithm>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace tf {
namespace {
void ClampIndex(int& idx, int size) {
  if (size <= 0) {
    idx = 0;
    return;
  }
  idx = std::max(0, std::min(idx, size - 1));
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
}  // namespace

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
  if (result.HasError()) {
    details_text_ = "Error: " + result.error().message;
    return;
  }

  const auto& b = result.value();
  details_text_ = fmt::format("ID: {}\nVersion: {}.{}\nType: {}\nTemplate: {}",
                              b.Id(), b.version().major, b.version().minor,
                              BlockTypeToString(b.type()), b.templ().Content());
}

void Tui::SelectBlockById(const std::string& block_id) {
  if (block_ids_.empty()) return;

  const auto it = std::ranges::find(block_ids_, block_id);
  if (it == block_ids_.end()) return;
  selected_block_ = static_cast<int>(std::distance(block_ids_.begin(), it));
}

void Tui::OnBlockPublished(const std::string& block_id) {
  RefreshBlocksList();
  SelectBlockById(block_id);
  UpdateSelectedBlockDetails();
  show_create_block_modal_ = false;
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

    // Edit mode publishes a new version of the selected block ID.
    block_creator_.BeginEdit(result.value());
    show_create_block_modal_ = true;
  });

  auto block_id_input = ftxui::Input(&block_creator_.BlockId(), "role.system");
  // Toggle avoids accidental "jump to last item" behavior seen with Menu
  // inside keyboard-driven vertical forms.
  auto block_type_toggle =
      ftxui::Toggle(&BlockTypeLabels(), &block_creator_.BlockTypeIndex());
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
    // Controller decides create vs edit behavior based on current mode.
    auto created_block_id = block_creator_.Create();
    if (!created_block_id.has_value()) return;
    OnBlockPublished(*created_block_id);
  });
  auto reset_button =
      ftxui::Button("Reset", [this] { block_creator_.Reset("Form reset"); });
  auto cancel_button =
      ftxui::Button("Cancel", [this] { show_create_block_modal_ = false; });

  auto create_buttons = ftxui::Container::Horizontal(
      {create_button, reset_button, cancel_button});
  auto create_form = ftxui::Container::Vertical(
      {block_id_input, block_type_toggle, block_template_input,
       block_defaults_input, block_tags_input, block_lang_input,
       block_desc_input, create_buttons});

  auto details = ftxui::Renderer([this] {
    return ftxui::window(
               ftxui::text(" Details "),
               ftxui::paragraph(details_text_) | ftxui::xflex | ftxui::yflex) |
           ftxui::xflex | ftxui::yflex;
  });

  auto create_panel = ftxui::Renderer(
      create_form,
      [this, block_id_input, block_type_toggle, block_template_input,
       block_defaults_input, block_tags_input, block_lang_input,
       block_desc_input, create_button, reset_button, cancel_button] {
        return ui::RenderCreateBlockForm(
            block_creator_.ModalTitle(), block_creator_.SubmitButtonLabel(),
            block_id_input, block_type_toggle, block_template_input,
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
        ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 36) | ftxui::yflex;
    auto actions = ftxui::window(
        ftxui::text(" Actions "),
        ftxui::vbox({open_create_modal_button->Render(), ftxui::separator(),
                     open_edit_modal_button->Render()}) |
            ftxui::flex);
    auto actions_sized =
        actions | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 24) | ftxui::yflex;

    return ftxui::hbox({
               left,
               details->Render() | ftxui::xflex | ftxui::yflex,
               actions_sized,
           }) |
           ftxui::xflex | ftxui::yflex;
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

}  // namespace tf
