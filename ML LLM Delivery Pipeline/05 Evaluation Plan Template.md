---
tags:
  - template
  - evaluation
  - quality
  - work
  - ml
  - llm
status: draft
date: <% tp.file.creation_date("YYYY-MM-DD") %>
---
# 05 Evaluation Plan: [Service Name]

**Status:** Draft / In Review / Approved  
**Owner:** [DS Lead]  
**Related Docs:** [01 PRD], [04 Service Spec], [09 Prompt Template]

---

## 1. Evaluation Goal (Цель оценки)

- **What quality means here:** [Как понимаем качество]
- **Main risks:** [Какие ошибки самые опасные]
- **Release decision this evaluation supports:** [Какое решение мы принимаем по итогам оценки]

## 2. Quality Dimensions (Измерения качества)

| Dimension | Why It Matters | Priority |
| :--- | :--- | :--- |
| [Faithfulness / Accuracy / Safety / Coverage] | [Why] | [High / Medium / Low] |

## 3. Acceptance Criteria (Критерии приемки)

| Dimension | Metric | Threshold | Method |
| :--- | :--- | :--- | :--- |
| [Quality] | [Metric] | [Target] | [How measured] |

## 4. Dataset Strategy (Стратегия датасетов)

- **Gold dataset:** [Источник и объем]
- **Test cases:** [Типы кейсов]
- **Negative cases:** [Что должно отфильтровываться / отклоняться]
- **Coverage gaps:** [Чего пока нет]

## 5. Offline Evaluation (Оффлайн-оценка)

- [Метод]
- [Метод]
- **Main benchmark:** [На чем меряем]

## 6. Manual Review (Ручной review)

- **Who reviews:** [Кто ревьюит]
- **Sample policy:** [Как выбирается выборка]
- **Disagreement handling:** [Что делать при расхождении оценок]

## 7. Thresholds and Decision Rule (Пороги и правило принятия решения)

- **Go threshold:** [Когда можно идти дальше]
- **No-go threshold:** [Когда выпускать нельзя]
- **Escalation zone:** [Когда нужен ручной разбор]

## 8. Regression Policy (Политика регрессии)

- **What counts as regression:** [Что считаем регрессией]
- **Comparison baseline:** [С чем сравниваем]
- **Blocking regressions:** [Какие регрессии блокируют релиз]

## 9. Post-Release Quality Review (Пересмотр качества после релиза)

- **Online signals:** [Feedback / production metrics]
- **Review cadence:** [Как часто пересматриваем]
- **Owners:** [Кто отвечает]
- **Update trigger:** [Когда пересматриваем thresholds / datasets / prompts]
