//
// Created by artem.d on 31.01.2026.
//

#include "../textfoundry_engine/block.h"
#include "../textfoundry_engine/engine.h"
#include <iostream>


int main() {
  const tf::EngineConfig config{.default_data_path = "memory:objectbox"};
  tf::Engine engine{config};

  auto greeting_result = engine.publish_block(
    tf::BlockDraftBuilder("greeting.formal")
    .with_template(tf::Template("Уважаемый {{name}}!"))
    .with_default("name", "клиент")
    .with_type(tf::BlockType::Role)
    .build()
  );

  if (greeting_result.has_error()) {
    return 1;
  }

  auto greeting = greeting_result.value();

  auto doc_result = engine.publish_composition(
    tf::CompositionDraftBuilder("welcome.doc")
    .with_project_key("crm")
    .add_block_ref(greeting.ref())
    .add_separator(tf::SeparatorType::Paragraph)
    .add_static_text("Добро пожаловать в систему.")
    .build()
  );

  if (doc_result.has_value()) {
    auto rendered = engine.render(doc_result.value().id(),
                                  doc_result.value().version());
    std::cout << rendered.value().text << std::endl;
  }
}
