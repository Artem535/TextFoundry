---
name: ml-llm-document-set
description: Use when the user wants to review completeness, gaps, or consistency across the full ML/LLM document set, especially links between PRD, system design, ADR, service spec, evaluation, runbook, security, and prompt artifacts.
---

# ML / LLM Document Set Review

Read:

* `../../../ML LLM Delivery Pipeline/ML LLM Delivery Pipeline and Required Documents.md`

Then read only the document files that exist in the project and are relevant to
the requested review.

## Check

* required artifacts are present
* upstream and downstream links are consistent
* owners and statuses are assigned
* scope in PRD matches system design and service spec
* evaluation thresholds match release claims
* runbook and security note reflect actual architecture
* prompt template matches service spec and eval assumptions

## Output

Prefer a gap report with:

* missing artifacts
* contradictions
* stale references
* next document to create or fix

