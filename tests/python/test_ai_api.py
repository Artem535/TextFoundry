import textfoundry as tf


def test_openai_configuration_is_explicit_and_deterministic():
    config = tf.OpenAiCompatibleConfig()
    config.base_url = "http://127.0.0.1:1/v1"
    config.model = "fixture"
    config.api_key = "secret"
    assert "secret" not in repr(config)

    engine = tf.Engine(data_path="memory:python-ai")
    engine.configure_openai(config)
    assert engine.has_block_generator()
    assert engine.has_block_normalizer()
    assert engine.has_composition_block_rewriter()

    block = (tf.BlockDraftBuilder("role.greeting")
             .with_type(tf.BlockType.Role)
             .with_template(tf.Template("Hello, {{name}}!"))
             .build())
    published = engine.publish_block(block)
    composition = (tf.CompositionDraftBuilder("demo")
                   .add_block_ref("role.greeting", published.version.major,
                                  published.version.minor)
                   .build())
    engine.publish_composition(composition)
    assert engine.render("demo", {"name": "Ada"}).text == "Hello, Ada!"


if __name__ == "__main__":
    test_openai_configuration_is_explicit_and_deterministic()
