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


if __name__ == "__main__":
    test_version()
    test_block_builder_fluent()
    test_error_boundary()
