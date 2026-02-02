// Created by artem.d on 31.01.2026.

#include "../textfoundry_engine/tf/block.h"
#include "../textfoundry_engine/tf/engine.h"
#include <iostream>

using namespace tf;

int main() {
  const EngineConfig config{.default_data_path = "memory:objectbox"};
  Engine engine{config};

  auto greeting_result = engine.publish_block(
    BlockDraftBuilder("greeting.formal")
    .with_template(Template("Уважаемый {{name}}!"))
    .with_default("name", "клиент")
    .with_type(BlockType::Role)
    .build(),
    Engine::VersionBump::Minor
  );

  if (!greeting_result.has_value()) {
    return 1;
  }

  auto greeting = greeting_result.value();

  auto doc_result = engine.publish_composition(
    CompositionDraftBuilder("welcome.doc")
    .with_project_key("crm")
    .add_block_ref(greeting.ref())
    .add_separator(SeparatorType::Paragraph)
    .add_static_text("Добро пожаловать в систему.")
    .build(),
    Engine::VersionBump::Minor
  );

  if (doc_result.has_value()) {
    auto rendered_result = engine.render(doc_result.value().id(),
                                  doc_result.value().version());
    if (rendered_result.has_value()) {
      std::cout << rendered_result.value().text << std::endl;
    }
  }

  return 0;
}
