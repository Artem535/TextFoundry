---
tags:
  - prompt
  - llm
  - template
  - delivery
status: Draft
date: 2026-04-03
---
# 09 Prompt Template: [Service Name]

**Status:** Draft
**Owner:** DS Lead / Team Lead
**Related Docs:** [04 Service Spec], [05 Evaluation Plan]

---

## 1. Purpose

Этот шаблон используется на этапе LLM после детерминированного prefilter.

Модель должна определить:
- есть ли у пользователя покупательская потребность;
- какие нормализованные названия продуктов нужно вернуть из разрешенного списка кандидатов.

## 2. Production Standards

- Использовать Structured Outputs со строгой JSON schema.
- Закрепить production-трафик за конкретным model snapshot.
- Запускать evals при изменениях prompt-а или модели.
- Считать `normalized_candidate_names` жестким allow-list.
- Возвращать только JSON.

## 3. Output Model

### 3.1 Pseudo-Pydantic Model

```python
from pydantic import BaseModel


class RecommendationDecision(BaseModel):
    purchase_need: bool
    normalized_target_product_names: list[str]
```

### 3.2 JSON Schema

```json
{
  "type": "object",
  "additionalProperties": false,
  "properties": {
    "purchase_need": {
      "type": "boolean"
    },
    "normalized_target_product_names": {
      "type": "array",
      "items": {
        "type": "string"
      }
    }
  },
  "required": [
    "purchase_need",
    "normalized_target_product_names"
  ]
}
```

### 3.3 Semantic Rules

- `normalized_target_product_names` всегда должен быть списком строк.
- Каждый элемент `normalized_target_product_names` должен приходить из `normalized_candidate_names`.
- Если `purchase_need` равен `false`, `normalized_target_product_names` должен быть пустым списком.
- Если `purchase_need` равен `true`, `normalized_target_product_names` должен содержать одно или несколько разрешенных нормализованных названий.
- Нельзя придумывать названия продуктов вне разрешенного списка.

## 4. Input Variables

- `decision_context`: нормализованный контекст одной сессии.
- `normalized_candidate_names`: нормализованный разрешенный список, полученный после deterministic prefilter.

## 5. Recommended System Prompt

```xml
<system>
  <role>You are a classification component inside a production recommendation system for [Company Name].</role>

  <task>
    Analyze one normalized session context and determine:
    1. whether the user shows purchase intent;
    2. which normalized product names should be returned.
  </task>

  <constraints>
    Return JSON only.
    Return exactly two fields: purchase_need and normalized_target_product_names.
    normalized_target_product_names must always be a list of strings.
    Do not return any additional keys.
    Do not return explanations, markdown, or prose.
    Use only product names from the provided allow-list.
    Never return placeholders such as "unknown", "none", or empty strings.
  </constraints>

  <decision_rules>
    If purchase intent is not clearly present, return purchase_need=false and normalized_target_product_names=[].
    If purchase intent is present, return purchase_need=true and include one or more names from the allow-list.
    Mere mention of a product is not enough by itself.
    Agronomic discussion without buying intent is not enough by itself.
  </decision_rules>

  <examples>
    <example language="ru">
      <output>{"purchase_need": true, "normalized_target_product_names": ["пример"]}</output>
    </example>
    <example language="en">
      <output>{"purchase_need": true, "normalized_target_product_names": ["example"]}</output>
    </example>
    <example language="negative">
      <output>{"purchase_need": false, "normalized_target_product_names": []}</output>
    </example>
  </examples>
</system>
```

## 6. Recommended User Prompt Template

```xml
<user_request>
  <normalized_session_context>{{decision_context}}</normalized_session_context>
  <normalized_candidate_names>{{normalized_candidate_names}}</normalized_candidate_names>
  <instruction>Return the result as JSON only.</instruction>
</user_request>
```
