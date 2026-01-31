//
// Created by artem.d on 31.01.2026.
//

#include "../textfoundry_engine/block.h"
#include "../textfoundry_engine/engine.h"
#include <iostream>


int main() {
  const tf::EngineConfig config{.default_data_path = "memory:objectbox"};
  tf::Engine engine{config};

  tf::Block block = engine.create_block_draft(tf::BlockId{"test.hello_world"});
  block.set_template(tf::Template("Hello World by {{name}}"));
  block.set_defaults({{"name", "Artem"}});

  const auto error = engine.save_block(block);
  const auto published = engine.publish_block("test.hello_world", {0, 0});
  const auto res = engine.render_block("test.hello_world", {0, 0}, {});
  std::cout << res.value() << std::endl;
}
