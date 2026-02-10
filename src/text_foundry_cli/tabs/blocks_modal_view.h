#pragma once

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
    const ftxui::Component& cancel_button, const std::string& status_text);

}  // namespace tf::ui
