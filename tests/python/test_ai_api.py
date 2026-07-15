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


def test_fake_transport_captures_generator_request_and_errors_offline():
    if not hasattr(tf, "_testing"):
        return
    fake = tf._testing.FakeTransport()
    fake.response = tf.HttpResponse(
        status_code=200,
        body=(
            '{"choices":[{"message":{"content":"{'
            '\\"id\\":\\"role.x\\",\\"type\\":\\"role\\",'
            '\\"language\\":\\"en\\",\\"description\\":\\"d\\",'
            '\\"templ\\":\\"Hi\\",\\"defaults\\":{},\\"tags\\":[]}"}}]}'
        ),
    )
    config = tf.OpenAiCompatibleConfig()
    config.base_url = "https://example.test/v1"
    config.model = "fixture"
    config.api_key = "secret"
    engine = tf.Engine(data_path="memory:python-ai-fake")
    engine.configure_openai(config, fake=fake)
    result = engine.generate_block_data(tf.BlockGenerationRequest(prompt="make one"))
    assert result.id == "role.x"
    assert fake.last_url == "https://example.test/v1/chat/completions"
    assert "response_format" in fake.last_body

    fake.response = tf.HttpResponse(status_code=500, body="{}")
    try:
        engine.generate_block_data(tf.BlockGenerationRequest(prompt="fail"))
    except tf.Error:
        pass
    else:
        raise AssertionError("expected textfoundry.Error")


if __name__ == "__main__":
    test_openai_configuration_is_explicit_and_deterministic()
    test_fake_transport_captures_generator_request_and_errors_offline()
