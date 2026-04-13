---
name: ml-llm-service-spec
description: "Use when the user wants the integration contract for an ML or LLM service: API/tool methods, input/output schema, data contract rules, prompt hierarchy, retries, confidence routing, and fallback behavior."
---

# ML / LLM Service Spec

Read:

* `../../../ML LLM Delivery Pipeline/04 Service Spec Template.md`

Use this skill when another team or service must integrate with the ML/LLM
component.

## Cover

* service boundary and responsibility
* consumers and dependencies
* API or tool methods
* input schema
* output schema
* data contract rules
* prompting and source hierarchy
* error handling and retries
* confidence and routing rules
* edge cases and fallback behavior

## Guardrails

* Be explicit about schema and contract behavior.
* Keep business context short and operational detail secondary.
