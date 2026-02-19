#include "../tui.h"
#include "blocks_modal_view.h"

#include <fmt/format.h>

#include <algorithm>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
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
  detail_id_.reset();
  detail_type_.reset();
  detail_version_.reset();
  detail_template_.reset();

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
  detail_id_ = b.Id();
  detail_version_ = fmt::format("{}.{}", b.version().major, b.version().minor);
  detail_type_ = std::string(BlockTypeToString(b.type()));
  detail_template_ = b.templ().Content();
  details_text_.clear();
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
    focus_block_modal_on_open_ = true;
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
    focus_block_modal_on_open_ = true;
  });

  auto block_id_input = ftxui::Input(&block_creator_.BlockId(), "role.system");
  // Toggle avoids accidental "jump to last item" behavior seen with Menu
  // inside keyboard-driven vertical forms.
  auto block_type_toggle =
      ftxui::Toggle(&BlockTypeLabels(), &block_creator_.BlockTypeIndex());
  auto template_input_option = ftxui::InputOption::Default();
  template_input_option.multiline = true;
  auto block_template_input = ftxui::Input(&block_creator_.BlockTemplate(),
                                           "Template with {{params}}",
                                           template_input_option);
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

  auto details = ftxui::Renderer([this](const bool focused) {
    if (!details_text_.empty()) {
      auto base = ftxui::window(
                 ftxui::text(" Details "),
                 ftxui::paragraph(details_text_) | ftxui::xflex |
                     ftxui::yflex) |
             ftxui::xflex | ftxui::yflex;
      return focused ? base | ftxui::borderStyled(ftxui::Color::Orange1) : base;
    }

    auto section = [&](const std::string& label, const std::string& value) {
      return ftxui::hbox({
          ftxui::text(label + ": ") | ftxui::bold,
          ftxui::text(value),
      });
    };

    auto template_value = detail_template_.value_or("");
    auto template_view = template_value.empty()
                             ? ftxui::text("(empty)") | ftxui::dim
                             : ftxui::paragraph(template_value) |
                                   ftxui::vscroll_indicator |
                                   ftxui::hscroll_indicator | ftxui::frame |
                                   ftxui::xframe;
    if (focused) {
      template_view = template_view | ftxui::focus;
    }

    auto base = ftxui::window(
               ftxui::text(" Details "),
               ftxui::vbox({
                   section("id", detail_id_.value_or("-")),
                   ftxui::separator(),
                   section("version", detail_version_.value_or("-")),
                   ftxui::separator(),
                   section("type", detail_type_.value_or("-")),
                   ftxui::separator(),
                   ftxui::text("template") | ftxui::bold,
                   template_view | ftxui::xflex | ftxui::yflex,
               }) |
                   ftxui::xflex | ftxui::yflex) |
           ftxui::xflex | ftxui::yflex;
    return focused ? base | ftxui::borderStyled(ftxui::Color::Orange1) : base;
  });

  auto create_panel = ftxui::Renderer(
      create_form,
      [this, block_id_input, block_type_toggle, block_template_input,
       block_defaults_input, block_tags_input, block_lang_input,
       block_desc_input, create_button, reset_button, cancel_button] {
        return ui::RenderCreateBlockForm(
            block_creator_.ModalTitle(), block_creator_.SubmitButtonLabel(),
            block_creator_.CurrentMode() == BlockCreationController::Mode::kEdit,
            block_id_input, block_type_toggle, block_template_input,
            block_creator_.BlockTemplate(),
            block_defaults_input, block_tags_input, block_lang_input,
            block_desc_input, create_button, reset_button, cancel_button,
            block_creator_.Status());
      });
  create_panel = create_panel | ftxui::CatchEvent(
                                    [this, block_id_input, block_type_toggle,
                                     block_template_input, block_defaults_input,
                                     block_tags_input, block_lang_input,
                                     block_desc_input, create_button,
                                     reset_button, cancel_button,
                                     modal_focus_chain =
                                         std::vector<ftxui::Component>{
                                             block_id_input,
                                             block_type_toggle,
                                             block_template_input,
                                             block_defaults_input,
                                             block_tags_input,
                                             block_lang_input,
                                             block_desc_input,
                                             create_button,
                                             reset_button,
                                             cancel_button,
                                         },
                                     modal_focus_index = 0](
                                        const ftxui::Event& e) mutable {
                                      if (!show_create_block_modal_) {
                                        return false;
                                      }

                                     if (focus_block_modal_on_open_) {
                                        modal_focus_index = 2;  // template
                                        modal_focus_chain[modal_focus_index]
                                            ->TakeFocus();
                                        focus_block_modal_on_open_ = false;
                                      }

                                      if (e != ftxui::Event::Tab &&
                                          e != ftxui::Event::TabReverse) {
                                        return false;
                                      }

                                      // Sync index with actual focused
                                      // component before moving.
                                      for (size_t i = 0;
                                           i < modal_focus_chain.size(); ++i) {
                                        if (modal_focus_chain[i]->Focused()) {
                                          modal_focus_index =
                                              static_cast<int>(i);
                                          break;
                                        }
                                      }

                                      if (e == ftxui::Event::Tab) {
                                        modal_focus_index =
                                            (modal_focus_index + 1) %
                                            static_cast<int>(
                                                modal_focus_chain.size());
                                      } else {
                                        modal_focus_index =
                                            (modal_focus_index - 1 +
                                             static_cast<int>(
                                                 modal_focus_chain.size())) %
                                            static_cast<int>(
                                                modal_focus_chain.size());
                                      }
                                      modal_focus_chain[modal_focus_index]
                                          ->TakeFocus();
                                      return true;
                                    });

  auto content = ftxui::Container::Horizontal(
      {list, details, open_create_modal_button, open_edit_modal_button});
  content =
      content | ftxui::CatchEvent([this](const ftxui::Event& e) {
        if (show_create_block_modal_) {
          return false;
        }
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
        ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 42) | ftxui::yflex;
    auto actions = ftxui::window(
        ftxui::text(" Actions "),
        ftxui::vbox({open_create_modal_button->Render(), ftxui::separator(),
                     open_edit_modal_button->Render()}) |
            ftxui::flex);
    auto actions_sized =
        actions | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 28) | ftxui::yflex;

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
      focus_block_modal_on_open_ = false;
      return true;
    }
    return false;
  });
}

}  // namespace tf
