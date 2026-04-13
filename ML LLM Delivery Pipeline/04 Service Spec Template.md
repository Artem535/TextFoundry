---
tags:
  - template
  - service-spec
  - contract
  - work
  - ml
  - llm
status: draft
date: <% tp.file.creation_date("YYYY-MM-DD") %>
---
# 04 Service Spec: [Service Name]

**Status:** Draft / In Review / Approved  
**Owner:** [Team Lead / DS Lead]  
**Related Docs:** [01 PRD], [02 System Design], [05 Evaluation Plan], [09 Prompt Template]

---

## 1. Purpose and Service Boundary (Назначение и границы сервиса)

- **Service responsibility:** [За что отвечает сервис]
- **Consumers:** [Кто вызывает сервис]
- **Non-goals:** [Что сервис не делает]
- **In scope:** [Что входит в зону ответственности]
- **Out of scope:** [Что точно не делает сервис]

## 2. Consumers and Dependencies (Потребители и зависимости)

### Consumers
- [Кто вызывает сервис]
- [Кто использует результат]

### Dependencies
- [Какой внешний сервис / база / модель используется]
- [Какие зависимости критичны]

## 3. API / Tool Methods (Методы API / инструментов)

| Method | Input | Output | Notes |
| :--- | :--- | :--- | :--- |
| [method] | [schema] | [schema] | [notes] |

## 4. Input Schema (Входная схема)

### Input Schema
```json
{
  "field": "type"
}
```

## 5. Output Schema (Выходная схема)

### Output Schema
```json
{
  "field": "type"
}
```

## 6. Data Contract Rules (Правила data contract)

- **Required fields:** [Какие поля обязательны]
- **Optional fields:** [Какие поля опциональны]
- **Validation rules:** [Какие проверки обязательны]
- **Versioning / compatibility:** [Как меняется контракт]

## 7. Prompting and Source Hierarchy (Промпты и иерархия источников)

### Prompt Inventory
- **System prompt:** [Назначение]
- **Developer / policy prompt:** [Назначение]
- **Task / extraction prompt:** [Назначение]
- **Review / correction prompt:** [Назначение, если есть]

### Prompt Variables
- [Какие переменные и контекст подставляются]
- [Какие поля обязательны]

- **Source hierarchy:** [FAQ > RAG > Expert / etc.]
- **Safety / refusal policy:** [Rules]
- **Clarification policy:** [When service asks for more input]
- **Fallback behavior:** [What happens on low confidence / timeout]

## 8. Error Handling and Retries (Обработка ошибок и повторы)

| Error Type | Condition | Expected Behavior |
| :--- | :--- | :--- |
| [error] | [when] | [behavior] |

- **Retry policy:** [Policy]
- **Timeouts:** [Values]
- **Rate limits:** [Limits]
- **Idempotency:** [Rule]

## 9. Confidence / Routing Rules (Правила confidence и маршрутизации)

| Signal | Source | Threshold | Action |
| :--- | :--- | :--- | :--- |
| [signal] | [source] | [threshold] | [action] |

## 10. Edge Cases and Fallback Behavior (Краевые случаи и fallback-поведение)

- [Case 1] -> [Expected behavior]
- [Case 2] -> [Expected behavior]
- [Case 3] -> [Expected behavior]
