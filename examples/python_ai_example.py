"""Offline AI-adapter example using the testing transport."""

import json

import textfoundry as tf


def completion(content: object) -> tf.HttpResponse:
    """Wrap a structured value in the OpenAI chat-completion envelope."""
    message_content = content if isinstance(content, str) else json.dumps(content)
    return tf.HttpResponse(
        status_code=200,
        body=json.dumps(
            {"choices": [{"message": {"role": "assistant", "content": message_content}}]}
        ),
    )


def main() -> None:
    # FakeTransport is exposed only in testing builds, so this script never
    # needs credentials and cannot make a network request.
    fake = tf._testing.FakeTransport()
    config = tf.OpenAiCompatibleConfig()
    config.base_url = "https://offline.example/v1"
    config.model = "fixture"
    config.api_key = "offline-only"
    engine = tf.Engine(data_path="memory:python-ai-example")
    engine.configure_openai(config, fake=fake)

    fake.response = completion(
        {
            "id": "role.greeting",
            "type": "role",
            "language": "en",
            "description": "A friendly greeting",
            "templ": "Hello, {{name}}!",
            "defaults": {},
            "tags": ["example"],
        }
    )
    generated = engine.generate_block_data(
        tf.BlockGenerationRequest(prompt="Create a greeting block")
    )
    print(f"generated block: {generated.id}")

    fake.response = completion(
        {
            "blocks": [
                {
                    "id": "role.greeting",
                    "type": "role",
                    "language": "en",
                    "description": "A reusable greeting",
                    "templ": "Hello, {{name}}!",
                    "defaults": {},
                    "tags": ["example"],
                }
            ]
        }
    )
    batch = engine.generate_block_batch_data(
        tf.PromptSlicingRequest(source_text="Greet the user by name.")
    )
    print(f"sliced blocks: {len(batch.blocks)}")

    style = tf.SemanticStyle()
    style.tone = "formal"
    fake.response = completion("Good day, {{name}}.")
    normalized = engine.normalize("Hello, {{name}}!", style)
    print(f"normalized text: {normalized}")

    block = (
        tf.BlockDraftBuilder("role.greeting")
        .with_type(tf.BlockType.Role)
        .with_template(tf.Template("Hello, {{name}}!"))
        .build()
    )
    published = engine.publish_block(block)
    composition = (
        tf.CompositionDraftBuilder("demo")
        .add_block_ref("role.greeting", published.version.major, published.version.minor)
        .build()
    )
    engine.publish_composition(composition)

    fake.response = completion(
        {
            "patches": [
                {
                    "block_id": "role.greeting",
                    "templ": "Good day, {{name}}.",
                    "rationale": "Use a formal greeting",
                }
            ]
        }
    )
    rewrite_request = tf.CompositionBlockRewriteRequest()
    rewrite_request.source_composition_id = "demo"
    rewrite_request.instruction = "Use a formal greeting"
    rewrite = engine.preview_composition_block_rewrite(rewrite_request)
    applied = engine.apply_composition_block_rewrite(rewrite)
    assert applied.composition_version.minor == 1
    print(f"rewritten composition: {applied.composition_id} v{applied.composition_version.to_string()}")


if __name__ == "__main__":
    main()
