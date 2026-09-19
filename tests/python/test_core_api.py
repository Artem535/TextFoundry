import textfoundry as tf


def test_version():
    assert tf.Version(2, 5).to_string() == "2.5"


def test_block_builder_fluent():
    b = tf.BlockDraftBuilder("role.greeter")
    assert b.with_template(tf.Template("Hello {{name}}")) is b
    assert b.with_defaults({"name": "world"}) is b
    assert isinstance(b.build(), tf.BlockDraft)


def test_error_boundary():
    try:
        tf.Template("Hello {{name}}").expand({})
    except tf.Error as exc:
        assert exc.code is tf.ErrorCode.MissingParam
        assert "name" in exc.message
    else:
        raise AssertionError("expected textfoundry.Error")


def test_composition_builder():
    draft = (tf.CompositionDraftBuilder("demo")
             .add_static_text("hello")
             .build())
    assert isinstance(draft, tf.CompositionDraft)
    assert tf.PublishedComposition.__module__ == "textfoundry"


def test_engine_render_workflow():
    engine = tf.Engine(data_path="memory:python-core")
    block = (tf.BlockDraftBuilder("role.greeting")
             .with_type(tf.BlockType.Role)
             .with_template(tf.Template("Hello, {{name}}!"))
             .build())
    published_block = engine.publish_block(block)
    composition = (tf.CompositionDraftBuilder("demo")
                   .add_block_ref("role.greeting",
                                  published_block.version.major,
                                  published_block.version.minor)
                   .build())
    engine.publish_composition(composition)
    result = engine.render("demo", {"name": "Ada"})
    assert result.text == "Hello, Ada!"


def test_render_block_missing_error():
    engine = tf.Engine(data_path="memory:python-core-missing")
    try:
        engine.render_block("missing")
    except tf.Error as exc:
        assert exc.code == tf.ErrorCode.BlockNotFound
    else:
        raise AssertionError("expected textfoundry.Error")


if __name__ == "__main__":
    test_version()
    test_block_builder_fluent()
    test_error_boundary()
    test_composition_builder()
    test_engine_render_workflow()
    test_render_block_missing_error()
