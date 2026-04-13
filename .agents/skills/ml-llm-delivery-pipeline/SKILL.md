---
name: ml-llm-delivery-pipeline
description: Use when the task is to create, update, sequence, or review the ML/LLM delivery document set as a pipeline rather than a single document. Covers PRD, system design, ADR, service spec, evaluation plan, delivery plan, runbook, security note, and prompt template.
---

# ML / LLM Delivery Pipeline

Use this skill when the user asks for a delivery-document workflow for an ML or
LLM system, wants to know what document comes next, or wants consistency across
multiple artifacts.

## Read First

Read:

* `../../../ML LLM Delivery Pipeline/ML LLM Delivery Pipeline and Required Documents.md`

Then read only the specific template files you need for the requested document.

## Document Order

Use this default sequence unless the user explicitly wants a subset:

1. `01 PRD`
2. `02 System Design Doc`
3. `03 ADR`
4. `04 Service Spec`
5. `05 Evaluation Plan`
6. `06 Delivery Plan`
7. `07 Runbook`
8. `08 Security Note`
9. `09 Prompt Template`

## Working Rules

* Treat the pipeline document as process guidance and the numbered files as the
  actual content templates.
* Keep cross-links between artifacts consistent.
* Reuse project-specific terminology from the repo instead of generic ML terms.
* If the repo already has canonical docs, adapt the pipeline output to the
  existing format instead of introducing a second parallel documentation system.
* When writing one artifact, name the upstream dependencies and downstream
  documents that must be updated next.

## Output Expectations

For each requested artifact:

* keep the original template intent
* replace placeholders with project-specific content
* remove sections that are clearly out of scope instead of leaving junk
* call out open questions explicitly when information is missing

