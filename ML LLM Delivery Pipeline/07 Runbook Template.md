---
tags:
  - template
  - runbook
  - operations
  - work
status: draft
date: <% tp.file.creation_date("YYYY-MM-DD") %>
---
# 07 Runbook: [Service Name]

**Status:** Draft / In Review / Approved  
**Owner:** [DevOps / Service Owner]  
**Related Docs:** [02 System Design], [06 Delivery Plan], [08 Security Note]

---

## 1. Operational Overview (Эксплуатационный обзор)

- **Service owner:** [Имя / команда]
- **Criticality:** [Low / Medium / High]
- **Support channel:** [Slack / email / on-call]

## 2. Ownership and Support Model (Модель владения и поддержки)

- **Primary owner:** [Кто отвечает за сервис]
- **Secondary owner / backup:** [Кто подхватывает]
- **Support model:** [On-call / business hours / ad hoc]
- **Escalation path:** [Куда эскалировать]

## 3. Key Signals and Alerts (Ключевые сигналы и алерты)

| Signal | Threshold | Alert Action |
| :--- | :--- | :--- |
| [Latency] | [Value] | [Action] |

## 4. Common Failure Modes (Типовые сценарии отказов)

| Failure Mode | Symptom | First Response |
| :--- | :--- | :--- |
| [Failure] | [Symptom] | [Action] |

## 5. Incident Response (Реакция на инциденты)

1. [Detect]
2. [Triage]
3. [Mitigate]
4. [Recover]
5. [Communicate]

## 6. Degradation Modes (Режимы деградации)

- [Fallback mode]
- [Read-only mode]
- [Disable optional dependency]

## 7. Recovery and Validation (Восстановление и проверка)

- [How to restart]
- [How to validate recovery]
- [What to check after restore]

## 8. Escalation Contacts (Контакты для эскалации)

- **DevOps:** [Контакт]
- **Team Lead:** [Контакт]
- **DS Lead / ML owner:** [Контакт]
- **Business / Project contact:** [Контакт]
