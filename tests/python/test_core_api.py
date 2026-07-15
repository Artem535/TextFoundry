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


if __name__ == "__main__":
    test_version()
    test_block_builder_fluent()
    test_error_boundary()
    test_composition_builder()
