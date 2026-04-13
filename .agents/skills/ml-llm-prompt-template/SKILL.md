---
name: ml-llm-prompt-template
description: Use when the user wants a production-grade prompt document for an ML or LLM service, including structured output contract, semantic rules, input variables, system prompt, and output examples.
---

# ML / LLM Prompt Template

Read:

* `../../../ML LLM Delivery Pipeline/09 Prompt Template.md`

Use this skill when documenting a production prompt as an operational artifact,
not just drafting prompt text ad hoc.

## Cover

* purpose of the prompt stage
* production standards
* output model
* JSON schema or structured output contract
* semantic rules
* input variables
* recommended system prompt
* examples and negative examples

## Guardrails

* Tie the prompt to an explicit schema.
* Record allow-lists, refusal rules, and output constraints.
* Treat prompt changes as versioned production changes, not casual edits.

