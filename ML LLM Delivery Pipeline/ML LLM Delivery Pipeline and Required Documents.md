---
tags:
  - process
  - delivery
  - ml
  - llm
  - architecture
  - methods
date: 2026-03-24
status: Draft
---
# ML / LLM Delivery Pipeline & Required Documents

**Owner:** Nikita Venediktov
**Status:** Draft
**Scope:** Какие документы нужны, чтобы ML-команда и команда LLM-сервисов могли передавать решение в разработку, интеграцию, эксплуатацию и поддержку без потери контекста.

---

## 1. Purpose

Этот документ описывает **delivery pipeline документации** для ML/LLM-проектов. Его задача не заменить `System Design Doc` или `ADR`, а зафиксировать:

- какие артефакты должны появиться по мере проектирования;
- в какой последовательности они нужны;
- что именно ML/LLM-команда обязана передать остальной команде;
- какой набор документов считается минимально достаточным для реализации и эксплуатации.

Под `PRD` в этом документе имеется в виду не обязательно полный PRD всего проекта. Чаще всего здесь нужен **сервисный PRD** или **PRD-lite**: компактный документ под конкретный сервис, capability или ML/LLM-пайплайн.

То есть:

- **Project PRD:** описывает весь продукт или большую инициативу целиком;
- **Service PRD / PRD-lite:** описывает конкретный сервис, агент, retriever, evaluation pipeline или internal tool.

Для большинства ML/LLM delivery-задач нужен именно **Service PRD / PRD-lite**.

---

## 2. Why This Matters

Для ML- и LLM-систем недостаточно одного большого "архитектурного документа". Остальной команде обычно нужны разные типы знаний:

- **Product / Business** должны понимать границы задачи и критерии успеха.
- **Backend / Platform** должны понимать контракты, зависимости, SLA и сценарии деградации.
- **Data / ML Ops** должны понимать пайплайны данных, версии моделей и evaluation.
- **QA** должны понимать ожидаемое поведение, test cases и acceptance criteria.
- **Support / Operations** должны понимать метрики, алерты, fallback-механику и runbooks.

Поэтому delivery должен строиться как **связка документов**, а не как один универсальный файл.

---

## 3. Recommended Document Set

Ниже приведен **сокращенный и канонический** набор документов. Идея в том, чтобы документов было меньше, но каждый был содержательнее и полезнее для команды.

| Prefix | Document          | Purpose                                                                                      | Main Owner           | Mandatory   |
| :----- | :---------------- | :------------------------------------------------------------------------------------------- | :------------------- | :---------- |
| `01`   | PRD / Service PRD | Формулирует бизнес-проблему, scope, KPI и ограничения                                        | Руководитель проекта | Yes         |
| `02`   | System Design Doc | Описывает целевую архитектуру системы end-to-end                                             | Архитектор           | Yes         |
| `03`   | ADR               | Фиксирует отдельные архитектурные решения и компромиссы                                      | Архитектор           | Yes         |
| `04`   | Service Spec      | Объединяет API Contract, Data Contract и Prompt Policy Spec                                  | Team Lead + DS Lead  | Yes         |
| `05`   | Evaluation Plan   | Объединяет качество, acceptance criteria и gold dataset                                      | DS Lead              | Yes         |
| `06`   | Delivery Plan     | Объединяет roadmap, backlog, dependency list и environment notes                             | Team Lead            | Yes         |
| `07`   | Runbook           | Объединяет operations, observability и incident response                                     | DevOps               | Yes         |
| `08`   | Security Note     | Фиксирует PII, data retention и policy constraints                                           | Архитектор + DevOps  | Recommended |
| `09`   | Prompt Template   | Фиксирует production-ready system prompt, user prompt, schema expectations и output examples | DS Lead + Team Lead  | Recommended |

### 3.1 Detailed Scope of Each Document

Ниже зафиксировано **согласованное содержание** каждого документа: какие вопросы он закрывает, какие секции обязательны, какое рекомендованное оглавление у будущего файла и что в него тащить не нужно.

#### `01` PRD / Service PRD

**Какие вопросы закрывает:**

- какую бизнес-проблему решает ML feature;
- кто пользователь и какой у него сценарий;
- что входит в scope, а что нет;
- как выглядит успех для бизнеса и продукта.

**Обязательные разделы:**

- проблема и цель сервиса;
- целевые пользователи и use cases;
- scope / non-goals;
- success criteria и KPI;
- ограничения и допущения;
- критерии результата для пилота и релиза.

**Рекомендуемая структура будущего файла:**

1. Goal and Business Context (Цель и бизнес-контекст)
2. Problem Statement (Формулировка проблемы)
3. Users and Use Cases (Пользователи и сценарии использования)
4. Scope and Non-Goals (Границы и что не входит)
5. Success Criteria and KPI (Критерии успеха и KPI)
6. Requirements Summary (Сводка требований)
7. Assumptions and Constraints (Допущения и ограничения)
8. Dependencies and Stakeholders (Зависимости и стейкхолдеры)
9. Pilot / Release Acceptance (Критерии приемки пилота / релиза)

**Что не должно жить здесь:**

- детальная архитектура;
- API и схемы данных;
- operational runbook;
- низкоуровневые ML-метрики и тестовые датасеты.

#### `02` System Design Doc

**Какие вопросы закрывает:**

- как система устроена end-to-end;
- какие есть компоненты и интеграции;
- как ходят данные и где узкие места;
- какие есть NFR, SLA/SLO и деградационные режимы.

**Обязательные разделы:**

- executive summary;
- контекст и границы решения;
- архитектурные диаграммы;
- компоненты и интеграции;
- data flow;
- workload и NFR;
- failure modes и fallback behavior.

**Рекомендуемая структура будущего файла:**

1. Executive Summary (Краткое резюме)
2. Context and Scope (Контекст и границы)
3. Requirements Summary (Сводка требований)
4. Decision Summary and Main Trade-offs (Сводка решения и ключевые компромиссы)
5. Workload and Operating Model (Нагрузка и модель эксплуатации)
6. Architecture Overview (Обзор архитектуры)
7. Components and Integrations (Компоненты и интеграции)
8. Key System Scenarios (Ключевые системные сценарии)
9. Data Flow (Потоки данных)
10. LLM / AI Design (LLM / AI дизайн)
11. Reliability and Degradation (Надежность и деградационные режимы)
12. Open Questions and Review Notes (Открытые вопросы и заметки ревью)
13. Operations Summary (Сводка по эксплуатации)

**Что не должно жить здесь:**

- бизнес-обоснование уровня PRD;
- детальное описание альтернатив по каждому спорному выбору;
- точные JSON-схемы;
- тестовый план и acceptance logic.

#### `03` ADR

**Какие вопросы закрывает:**

- почему выбрали именно это решение;
- какие альтернативы рассматривали;
- какие trade-offs приняли сознательно.

**Обязательные разделы:**

- контекст;
- альтернативы;
- решение;
- архитектурный срез выбранного решения;
- последствия;
- статус и история изменения.

**Рекомендуемая структура будущего файла:**

1. Context (Контекст)
2. Alternatives Considered (Рассмотренные альтернативы)
3. Decision (Принятое решение)
4. Chosen Architecture Slice (Архитектурный срез выбранного решения)
5. Consequences (Последствия)
6. Risks and Mitigations (Риски и митигации)
7. Links to Impacted Documents (Связанные и затронутые документы)
8. Lifecycle (Жизненный цикл решения)

**Что не должно жить здесь:**

- полный системный дизайн;
- все API-контракты;
- rollout-план;
- подробная эксплуатационная инструкция.

#### `04` Service Spec

**Какие вопросы закрывает:**

- как другой сервис или команда должны интегрироваться с ML feature;
- какой у сервиса вход, выход и контракт;
- как сервис должен вести себя в edge cases;
- какие policy rules влияют на поведение модели.

**Обязательные разделы:**

- назначение сервиса;
- API / tool methods;
- input / output schemas;
- data contract;
- промпты, иерархия источников и prompt inventory;
- timeout / retries / error handling;
- edge cases и fallback behavior.

**Рекомендуемая структура будущего файла:**

1. Purpose and Service Boundary (Назначение и границы сервиса)
2. Consumers and Dependencies (Потребители и зависимости)
3. API / Tool Methods (Методы API / инструментов)
4. Input Schema (Входная схема)
5. Output Schema (Выходная схема)
6. Data Contract Rules (Правила data contract)
7. Prompting and Source Hierarchy (Промпты и иерархия источников)
8. Error Handling and Retries (Обработка ошибок и повторы)
9. Confidence / Routing Rules (Правила confidence и маршрутизации)
10. Edge Cases and Fallback Behavior (Краевые случаи и fallback-поведение)

**Что не должно жить здесь:**

- бизнес-цели и KPI уровня PRD;
- полная архитектурная мотивация;
- методология evaluation;
- эксплуатационные инструкции по инцидентам.

**Что именно про промпты живет здесь:**

- сами prompt texts, если их объем остается управляемым;
- какие prompt blocks использует сервис;
- за что отвечает каждый prompt;
- какие переменные и контекст в него подставляются;
- какие prompt-level правила обязательны для поведения сервиса.

#### `05` Evaluation Plan

**Какие вопросы закрывает:**

- что считается хорошим качеством для этой ML feature;
- как это измеряется;
- на каких данных и по каким порогам принимается решение;
- когда решение готово к пилоту или релизу.

**Обязательные разделы:**

- quality dimensions;
- acceptance criteria;
- gold dataset / benchmark strategy;
- offline evaluation;
- manual review;
- thresholds и decision rule;
- post-release review cadence.

**Рекомендуемая структура будущего файла:**

1. Evaluation Goal (Цель оценки)
2. Quality Dimensions (Измерения качества)
3. Acceptance Criteria (Критерии приемки)
4. Dataset Strategy (Стратегия датасетов)
5. Offline Evaluation (Оффлайн-оценка)
6. Manual Review (Ручной review)
7. Thresholds and Decision Rule (Пороги и правило принятия решения)
8. Regression Policy (Политика регрессии)
9. Post-Release Quality Review (Пересмотр качества после релиза)

**Что не должно жить здесь:**

- описание API;
- архитектурные диаграммы;
- backlog и delivery-этапы;
- operational alerting.

#### `06` Delivery Plan

**Какие вопросы закрывает:**

- кто, что и в какой последовательности делает;
- какие есть зависимости между ролями и командами;
- как будет проходить rollout;
- что является delivery-ready состоянием.

**Обязательные разделы:**

- scope поставки;
- work breakdown по ролям;
- зависимости и блокеры;
- rollout plan;
- readiness checklist;
- ключевые риски внедрения.

**Рекомендуемая структура будущего файла:**

1. Delivery Scope (Объем поставки)
2. Workstreams and Owners (Потоки работ и владельцы)
3. Dependency Map (Карта зависимостей)
4. Rollout Plan (План rollout)
5. Risks and Blockers (Риски и блокеры)
6. Readiness Checklist (Чеклист готовности)
7. Migration / Transition Notes (Заметки по миграции / переходу)
8. Open Questions (Открытые вопросы)

**Что не должно жить здесь:**

- детальное устройство системы;
- точные ML-критерии качества;
- operational troubleshooting steps;
- подробные compliance-ограничения.

#### `07` Runbook

**Какие вопросы закрывает:**

- как сопровождать сервис в эксплуатации;
- по каким сигналам понимать, что сервис деградировал;
- что делать при инциденте;
- как восстанавливать сервис и проверять recovery.

**Обязательные разделы:**

- service ownership;
- key signals и alerts;
- common failure modes;
- incident response;
- degradation modes;
- recovery steps.

**Рекомендуемая структура будущего файла:**

1. Operational Overview (Эксплуатационный обзор)
2. Ownership and Support Model (Модель владения и поддержки)
3. Key Signals and Alerts (Ключевые сигналы и алерты)
4. Common Failure Modes (Типовые сценарии отказов)
5. Incident Response (Реакция на инциденты)
6. Degradation Modes (Режимы деградации)
7. Recovery and Validation (Восстановление и проверка)
8. Escalation Contacts (Контакты для эскалации)

**Что не должно жить здесь:**

- бизнес-контекст и KPI;
- архитектурные альтернативы;
- полное описание контрактов;
- план разработки фичи.

#### `08` Security Note

**Какие вопросы закрывает:**

- какие данные считаются чувствительными;
- где и как они хранятся;
- какие есть ограничения на логирование и вендоров;
- какие security/compliance риски известны.

**Обязательные разделы:**

- data sensitivity;
- storage and retention;
- access model;
- vendor constraints;
- compliance notes;
- risks and mitigations.

**Рекомендуемая структура будущего файла:**

1. Data Sensitivity (Чувствительность данных)
2. Storage and Retention (Хранение и retention)
3. Access Model (Модель доступа)
4. Logging Restrictions (Ограничения на логирование)
5. Vendor and Compliance Constraints (Ограничения по вендорам и compliance)
6. Security Controls (Контроли безопасности)
7. Risks and Mitigations (Риски и митигации)

**Что не должно жить здесь:**

- общий системный дизайн;
- детализация user journeys;
- acceptance criteria качества;
- operational runbook целиком.

#### `09` Prompt Template

**Какие вопросы закрывает:**

- какой production-ready system prompt использует сервис;
- какой user prompt template или prompt slots нужны на входе;
- какой structured output, schema expectations и examples ожидаются от LLM;
- какие prompt-level guardrails и formatting rules обязательны.

**Обязательные разделы:**

- system prompt;
- prompt inputs and variables;
- expected output schema;
- examples;
- guardrails and refusal rules;
- versioning and ownership.

**Рекомендуемая структура будущего файла:**

1. Prompt Purpose and Scope (Назначение и границы промпта)
2. System Prompt (Системный промпт)
3. Input Variables and Context Blocks (Переменные и блоки контекста)
4. Output Contract and Schema Expectations (Ожидаемый output contract и schema)
5. Guardrails and Refusal Rules (Ограничения и правила отказа)
6. Recommended User Prompt Template (Рекомендуемый user prompt template)
7. Examples (Примеры)
8. Versioning and Change Notes (Версионирование и история изменений)

**Что не должно жить здесь:**

- полная бизнес-мотивация уровня PRD;
- end-to-end архитектура;
- полный evaluation process;
- operational response и incident handling.

### 3.2 Coverage Matrix: Which Questions Are Answered Where

| Question | Primary Document | Supporting Documents |
| :--- | :--- | :--- |
| Зачем вообще нужна эта ML feature? | `01 PRD` | `06 Delivery Plan` |
| Кто пользователь и какой у него сценарий? | `01 PRD` | `02 System Design Doc` |
| Как система устроена целиком? | `02 System Design Doc` | `03 ADR` |
| Почему выбрали именно такую архитектуру? | `03 ADR` | `02 System Design Doc` |
| Как интегрироваться с сервисом? | `04 Service Spec` | `02 System Design Doc` |
| Какие входы, выходы и схемы данных? | `04 Service Spec` | `08 Security Note` |
| Какие policy и guardrails управляют поведением? | `04 Service Spec` | `08 Security Note`, `09 Prompt Template` |
| Как понять, что качество приемлемо? | `05 Evaluation Plan` | `01 PRD` |
| Кто и когда внедряет это в проект? | `06 Delivery Plan` | `02 System Design Doc` |
| Как поддерживать сервис в проде? | `07 Runbook` | `08 Security Note` |
| Какие ограничения по данным и безопасности? | `08 Security Note` | `07 Runbook`, `04 Service Spec`, `09 Prompt Template` |
| Какой production prompt и какой output contract использует LLM? | `09 Prompt Template` | `04 Service Spec`, `05 Evaluation Plan` |

### 3.3 Minimal Cross-Document Rules

Чтобы документы не дублировали друг друга, полезно держать такие правила:

- `PRD` отвечает за **зачем** и **что для бизнеса важно**.
- `System Design Doc` отвечает за **как система устроена end-to-end**.
- `ADR` отвечает за **почему решение принято именно так**.
- `Service Spec` отвечает за **как с сервисом взаимодействовать и как он должен себя вести**.
- `Evaluation Plan` отвечает за **как измеряется качество и принимается решение о готовности**.
- `Delivery Plan` отвечает за **как решение доезжает до реализации и rollout**.
- `Runbook` отвечает за **как жить с сервисом после запуска**.
- `Security Note` отвечает за **какие ограничения на данные, доступ и вендоров нельзя нарушать**.
- `Prompt Template` отвечает за **какой production prompt используется и какой structured output ожидается от LLM**.

---

## 4. Delivery Pipeline

### Stage 1. Problem Framing

На этом этапе команда должна зафиксировать, что именно мы строим и зачем.

**Required documents:**

- `PRD`

**Output of the stage:**

- есть общая формулировка бизнес-задачи;
- понятны критерии успеха;
- зафиксированы границы MVP;
- понятно, почему нужен именно ML/LLM-подход, а не rule-based automation.

### Stage 2. System Design

На этом этапе появляется целевая схема системы.

**Required documents:**

- `System Design Doc`

**Output of the stage:**

- backend-команда понимает состав компонентов;
- platform-команда понимает интеграции, нагрузку и эксплуатационные требования;
- QA понимает критические пользовательские и системные потоки.

### Stage 3. Decision Capture

На этом этапе фиксируются спорные и значимые архитектурные выборы.

**Required documents:**

- `ADR` по каждому значимому решению

**Examples:**

- выбор orchestrator vs fixed chain;
- выбор FAQ storage strategy;
- выбор memory strategy;
- выбор model provider;
- выбор retrieval architecture.

**Output of the stage:**

- у команды есть история решений;
- понятны альтернативы и trade-offs;
- изменения в архитектуре можно отслеживать без переписывания всего system design.

### Stage 4. Interface Definition

На этом этапе ML/LLM-команда должна перестать говорить только "идеями" и передать конкретные контракты.

**Required documents:**

- `Service Spec`
- `Prompt Template`, если в системе есть значимый LLM-шаг с production prompt

**Output of the stage:**

- backend может реализовывать интеграцию независимо;
- frontend / consuming services понимают формат ответов;
- QA получает проверяемые интерфейсы;
- снижается риск "LLM магии без контракта".

### Stage 5. Quality Definition

На этом этапе фиксируется, что считается "хорошим" качеством.

**Required documents:**

- `Evaluation Plan`
- `Prompt Template`, если качество зависит от конкретного prompt contract и примеров

**Output of the stage:**

- ML знает, как валидировать качество модели и пайплайна;
- product знает, как будет приниматься решение;
- QA получает основу для acceptance testing;
- backend понимает ожидаемое поведение fallback и refusal cases.

### Stage 6. Implementation Handoff

На этом этапе решение должно быть готово к передаче в delivery.

**Required documents:**

- `Delivery Plan`

**Output of the stage:**

- работу можно параллелить между командами;
- понятны зависимости между ML, backend, infra и QA;
- можно планировать релизы и этапы интеграции.

### Stage 7. Operational Readiness

На этом этапе система готовится к запуску и сопровождению.

**Required documents:**

- `Runbook`
- `Security Note`
- `Prompt Template`, если prompt changes и guardrails нужно контролировать как production artifact

**Output of the stage:**

- service owner понимает, как поддерживать систему;
- support знает, что делать при деградации;
- platform знает, какие метрики и алерты обязательны;
- бизнес понимает допустимое поведение в fallback-режимах.

---

## 5. Minimum Handoff Package from ML / LLM Team

Минимальный пакет, который команда должна передать дальше по delivery-цепочке, должен быть согласован с каноническим набором документов из раздела `3`.

### 5.1 Minimum Handoff Package

1. **PRD / Service PRD**
   Объясняет, зачем существует сервис, какую проблему он решает, какие у него scope, KPI и ограничения.
2. **System Design Doc**
   Описывает систему end-to-end, ее модули, data flow, ограничения и NFR.
3. **ADR**
   Фиксирует ключевые архитектурные решения, которые нельзя потерять при реализации и дальнейшем развитии.
4. **Service Spec**
   Описывает API, data contract, prompt policy, validation rules, response schemas и expected behavior сервиса.
5. **Evaluation Plan**
   Описывает, как будет измеряться качество и по каким критериям решение считается приемлемым.
6. **Delivery Plan**
   Описывает этапы внедрения, зависимости, порядок реализации и rollout.
7. **Runbook**
   Описывает, как сервис эксплуатируется, когда считается degraded и что делать при проблемах.
8. **Security Note**
   Описывает чувствительные данные, ограничения по логированию, retention, vendor constraints и security risks.
9. **Prompt Template**
   Описывает production-ready system prompt, input variables, output contract, guardrails и примеры.

Без этих восьми артефактов handoff считается базово неполным, а для LLM-сервиса полноценный handoff почти всегда требует и `09 Prompt Template`.

### 5.2 Lean Mode for Teams

Чтобы команда не пугалась объема документации, полезно явно разделять:

- **Lean mode:** минимальный набор документов для старта и пилота.
- **Full mode:** полный набор документов для production-ready delivery.

**Lean mode** может состоять из 5 документов:

1. `01-prd.md`
2. `02-system-design.md`
3. `03-adr-001-*.md` и другие ADR по мере необходимости
4. `04-service-spec.md`
5. `05-evaluation-plan.md`

Для большинства пилотов этого уже достаточно, чтобы команда начала двигаться без ощущения бюрократии.

Если пилот реально использует LLM в продовом или квази-продовом сценарии, добавляйте и `09-prompt-template.md` уже в lean mode.

**Full mode**

`06-delivery-plan.md`, `07-runbook.md`, `08-security-note.md` и `09-prompt-template.md` можно добавлять ближе к интеграции и production rollout, если они не были оформлены раньше.

---

## 6. Cross-Document Numbering

Чтобы документы было легко искать, сортировать и обсуждать, имеет смысл ввести **сквозную нумерацию по типам артефактов**.

### 6.1 Recommended Prefixes

| Prefix | Canonical Document Name | Example                       |
| :----- | :---------------------- | :---------------------------- |
| `01` | PRD / Service PRD       | `01-prd.md`                 |
| `02` | System Design Doc       | `02-system-design.md`       |
| `03` | ADR                     | `03-adr-001-faq-storage.md` |
| `04` | Service Spec            | `04-service-spec.md`        |
| `05` | Evaluation Plan         | `05-evaluation-plan.md`     |
| `06` | Delivery Plan           | `06-delivery-plan.md`       |
| `07` | Runbook                 | `07-runbook.md`             |
| `08` | Security Note           | `08-security-note.md`       |
| `09` | Prompt Template         | `09-prompt-template.md`     |

Эти prefixes должны использоваться во всех проектах одинаково, даже если часть документов в конкретной инициативе не создается в `Lean mode`.

### 6.2 Naming Rule

Правило простое:

- первые две цифры обозначают класс документа в pipeline;
- внутри класса можно использовать дополнительную нумерацию;
- название после номера должно быть коротким и понятным.

**Examples:**

- `01-prd-agro-agent.md`
- `02-system-design-agro-agent.md`
- `03-adr-001-orchestrator-vs-chain.md`
- `03-adr-002-faq-storage-strategy.md`
- `04-service-spec-agro-agent.md`
- `05-evaluation-plan-faq-rag.md`
- `09-prompt-template-agro-agent.md`
- `06-delivery-plan-agro-agent.md`
- `07-runbook-agro-agent.md`

### 6.3 Why This Helps

Такая схема дает несколько преимуществ:

- документы естественно сортируются в файловой системе и Obsidian;
- на созвонах проще ссылаться на артефакты по короткому номеру;
- команде легче понимать, каких документов еще не хватает;
- уменьшается хаос, когда документов становится много.

---

## 7. Document Ownership Matrix

| Artifact          | Primary Owner                           | Co-owners                                                              | Consumer                                               |
| :---------------- | :-------------------------------------- | :--------------------------------------------------------------------- | :----------------------------------------------------- |
| PRD / Service PRD | Руководитель проекта | Аналитик, Team Lead                                            | Delivery Team                                          |
| System Design Doc | Архитектор                    | Team Lead, DS Lead                                                     | Delivery Team                                          |
| ADR               | Архитектор                    | Team Lead, DS Lead                                                     | Delivery Team                                          |
| Service Spec      | Team Lead                               | Архитектор, Разработчики, DS Lead, DS            | Разработчики, QA, DevOps                   |
| Evaluation Plan   | DS Lead                                 | DS, QA, Team Lead                                                      | Руководитель проекта, Delivery Team |
| Delivery Plan     | Team Lead                               | Руководитель проекта, Архитектор, DS Lead | Delivery Team                                          |
| Runbook           | DevOps                                  | Team Lead, Разработчики, DS Lead                           | Support, QA, Delivery Team                             |
| Security Note     | DevOps                                  | Team Lead, Архитектор, DS Lead                               | Delivery Team                                          |

### 7.1 Role Interpretation for This Pipeline

Чтобы не было путаницы, ниже зафиксирована интерпретация ролей именно для этого процесса:

- **Руководитель проекта:** отвечает за согласование scope, сроков, приоритетов и внешних ожиданий.
- **Аналитик:** помогает формализовать бизнес-требования, сценарии использования и acceptance criteria.
- **Team Lead:** отвечает за delivery целиком, сборку handoff-пакета и синхронизацию всех ролей.
- **Архитектор:** отвечает за целостность системного дизайна и архитектурные решения.
- **DS Lead:** отвечает за ML/LLM-часть, качество, prompts/policies, evaluation и data contracts.
- **DS:** помогает готовить evaluation, датасеты, эксперименты и ML-артефакты.
- **Разработчики:** реализуют сервисы, интеграции и прикладные контракты.
- **QA:** участвует в acceptance criteria, тестовых сценариях и проверке качества поставки.
- **DevOps:** отвечает за operational readiness, окружения, наблюдаемость и эксплуатацию.

### 7.2 Practical Responsibility Rule

Удобное рабочее правило для команды:

- **Руководитель проекта** отвечает за вопрос "зачем и к какому сроку".
- **Архитектор** отвечает за вопрос "как система устроена".
- **DS Lead** отвечает за вопрос "как работает ML/LLM-логика и как измеряется качество".
- **Team Lead** отвечает за вопрос "как все это доедет до delivery без потери контекста".

Такое разделение обычно снимает конфликт "кто должен писать документ" и помогает не перекладывать ownership между ролями.

---

## 8. Suggested Structure of the Documentation Bundle

Практичная связка документов для одного ML/LLM-сервиса:

```text
project/
  01-discovery/
    01-prd.md
  02-design/
    02-system-design.md
    03-adr-001-*.md
    03-adr-002-*.md
  03-contracts/
    04-service-spec.md
  04-quality/
    05-evaluation-plan.md
  05-delivery/
    06-delivery-plan.md
  06-operations/
    07-runbook.md
  07-security/
    08-security-note.md
```

Такой набор хорошо разделяет:

- общий контекст;
- архитектуру;
- решения;
- интерфейсы;
- качество;
- внедрение и эксплуатацию.

Папки здесь полезны не только для порядка, но и как **этапы заполнения**:

- команда начинает с `01-discovery`;
- затем переходит в `02-design`;
- потом заполняет `03-contracts`;
- только после этого добавляет `04-quality`, `05-delivery` и `06-operations`, `07-security`.

То есть человек видит не "12 документов сразу", а последовательность шагов.

---

## 9. Recommended Sequence of Work

Рекомендуемая последовательность работы:

1. Сначала зафиксировать `PRD`.
2. Затем подготовить `System Design Doc`.
3. После этого выделить отдельные `ADR` по спорным архитектурным решениям.
4. Затем зафиксировать `Service Spec`.
5. Параллельно с проектированием оформить `Evaluation Plan`.
6. Перед интеграцией и rollout подготовить `Delivery Plan`.
7. Для LLM-сценариев зафиксировать `Prompt Template` до production readiness.
8. Перед production readiness подготовить `Runbook` и `Security Note`.

---

## 10. Practical Rule

Короткое правило применения:

- если нужно объяснить **зачем** нужен сервис, открываем `PRD`;
- если нужно объяснить **как система устроена**, открываем `System Design Doc`;
- если нужно объяснить **почему выбрали именно такое решение**, открываем `ADR`;
- если нужно объяснить **как интегрироваться с сервисом**, открываем `Service Spec`;
- если нужно объяснить **как измеряется качество**, открываем `Evaluation Plan`;
- если нужно объяснить **как внедрять и раскатывать**, открываем `Delivery Plan`;
- если нужно объяснить **как поддерживать в эксплуатации**, открываем `Runbook`;
- если нужно объяснить **ограничения по безопасности и данным**, открываем `Security Note`.
- если нужно объяснить **какой prompt и какой output contract использует LLM**, открываем `Prompt Template`.

---

## 11. Handoff Readiness Checklist

Передача ML/LLM-решения дальше по delivery-цепочке считается готовой, если выполнены следующие пункты:

- [ ] Зафиксирован `PRD` с понятными scope, non-goals и success criteria.
- [ ] Подготовлен актуальный `System Design Doc`.
- [ ] Созданы `ADR` по ключевым архитектурным решениям.
- [ ] Описан `Service Spec` с интерфейсами, схемами и expected behavior.
- [ ] Подготовлен `Evaluation Plan` с quality thresholds и acceptance logic.
- [ ] Подготовлен `Delivery Plan` с rollout и зависимостями.
- [ ] Подготовлен `Runbook` с observability и incident response.
- [ ] Подготовлен `Security Note`, если сервис работает с чувствительными данными, внешними вендорами или production-контуром.
- [ ] Подготовлен `Prompt Template`, если сервис опирается на production prompt, structured output или prompt-level guardrails.
- [ ] Назначены owners на сопровождение после запуска.

Если часть чеклиста не выполнена, handoff лучше считать частичным, а не завершенным.
