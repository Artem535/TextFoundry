#include "blocks_modal_view.h"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include <string>

namespace tf::ui {

ftxui::Element RenderCreateBlockForm(
    const std::string& modal_title, const std::string& submit_button_label,
    const ftxui::Component& block_id_input,
    const ftxui::Component& block_type_toggle,
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
                 block_type_toggle->Render(),
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

}  // namespace tf::ui
