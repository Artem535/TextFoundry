//
// Created by artem.d on 31.01.2026.
//

#include "../textfoundry_engine/block.h"
#include "../textfoundry_engine/engine.h"
#include <iostream>


int main() {
  const tf::EngineConfig config{.default_data_path = "memory:objectbox"};
  tf::Engine engine{config};

  tf::Block block = engine.createBlockDraft(tf::BlockId{"test.hello_world"});
  block.setTemplate(tf::Template("Hello World by {{name}}"));
  block.setDefaults({{"name", "Artem"}});

  const auto error = engine.saveBlock(block);
  const auto published = engine.publishBlock("test.hello_world", {0, 0});
  const auto res = engine.renderBlock("test.hello_world", {0, 0}, {});
  std::cout << res.value() << std::endl;
}
