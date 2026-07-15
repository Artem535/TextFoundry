"""Minimal deterministic TextFoundry Python API example."""

import textfoundry as tf


def main() -> None:
    engine = tf.Engine(data_path="memory:python-example")

    block = (
        tf.BlockDraftBuilder("role.greeting")
        .with_type(tf.BlockType.Role)
        .with_template(tf.Template("Hello, {{name}}!"))
        .build()
    )
    published_block = engine.publish_block(block)

    composition = (
        tf.CompositionDraftBuilder("demo")
        .add_block_ref(
            "role.greeting",
            published_block.version.major,
            published_block.version.minor,
        )
        .build()
    )
    engine.publish_composition(composition)

    result = engine.render("demo", {"name": "Ada"})
    print(result.text)


if __name__ == "__main__":
    main()
