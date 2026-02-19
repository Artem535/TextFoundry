#include "blocks_modal_view.h"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include <string>

namespace tf::ui {

ftxui::Element RenderCreateBlockForm(
    const std::string& modal_title, const std::string& submit_button_label,
    const bool is_edit_mode,
    const ftxui::Component& block_id_input,
    const ftxui::Component& block_type_toggle,
    const ftxui::Component& block_template_input,
    const std::string& block_template_text,
    const ftxui::Component& block_defaults_input,
    const ftxui::Component& block_tags_input,
    const ftxui::Component& block_lang_input,
    const ftxui::Component& block_desc_input,
    const ftxui::Component& create_button, const ftxui::Component& reset_button,
    const ftxui::Component& cancel_button, const std::string& status_text) {
  const auto terminal_size = ftxui::Terminal::Size();
  const int modal_width = std::max(80, terminal_size.dimx * 9 / 10);
  const int modal_height = std::max(24, terminal_size.dimy * 9 / 10);

  auto metadata_column = ftxui::vbox({
      ftxui::text("id") | ftxui::dim,
      block_id_input->Render(),
      ftxui::separator(),
      ftxui::text("type") | ftxui::dim,
      block_type_toggle->Render(),
      ftxui::separator(),
      ftxui::text("defaults (key=value, comma separated)") | ftxui::dim,
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
  });

  auto template_preview = block_template_text.empty()
                              ? ftxui::text("Template preview will appear here") |
                                    ftxui::dim
                              : ftxui::paragraph(block_template_text);

  auto editor_panel =
      ftxui::window(ftxui::text(" Template Editor "),
                    block_template_input->Render() | ftxui::focusCursorBar |
                        ftxui::vscroll_indicator | ftxui::hscroll_indicator |
                        ftxui::frame | ftxui::xframe | ftxui::xflex |
                        ftxui::yflex) |
      ftxui::xflex | ftxui::yflex;

  auto preview_panel =
      ftxui::window(ftxui::text(" Template Preview "),
                    template_preview | ftxui::vscroll_indicator |
                        ftxui::hscroll_indicator | ftxui::frame |
                        ftxui::xframe | ftxui::xflex | ftxui::yflex) |
      ftxui::xflex | ftxui::yflex;

  auto editor_column = ftxui::vbox({
      editor_panel | ftxui::yflex,
      ftxui::separator(),
      preview_panel | ftxui::yflex,
  });

  auto title = ftxui::text(modal_title) | ftxui::bold;

  auto modal = ftxui::vbox({
                   title | ftxui::center,
                   ftxui::separator(),
                   ftxui::hbox({
                       metadata_column |
                           ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 48) |
                           ftxui::yflex,
                       ftxui::separator(),
                       editor_column | ftxui::xflex | ftxui::yflex,
                   }) |
                       ftxui::xflex | ftxui::yflex,
                   ftxui::separator(),
                   ftxui::hbox({ftxui::text(submit_button_label + ": "),
                                create_button->Render(), ftxui::text(" "),
                                reset_button->Render(), ftxui::text(" "),
                                cancel_button->Render()}),
                   ftxui::separator(),
                   ftxui::paragraph(status_text) | ftxui::xflex |
                       ftxui::color(ftxui::Color::LightSteelBlue),
               }) |
               ftxui::xflex | ftxui::yflex;

  modal = modal | ftxui::borderStyled(ftxui::ROUNDED, ftxui::Color::Orange1);

  return modal | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, modal_width) |
         ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, modal_height) |
         ftxui::xflex | ftxui::yflex | ftxui::center;
}

}  // namespace tf::ui
