---
tags:
  - template
  - security
  - compliance
  - work
status: draft
date: <% tp.file.creation_date("YYYY-MM-DD") %>
---
# 08 Security Note: [Service Name]

**Статус:** Draft / In Review / Approved  
**Owner:** [Архитектор / DevOps]  
**Связанные документы:** [01 PRD], [02 System Design], [07 Runbook], [09 Prompt Template]

---

## 1. Data Sensitivity (Чувствительность данных)

- **PII / sensitive data:** [Что считается чувствительным]
- **Data sources:** [Откуда данные приходят]
- **Data consumers:** [Кто их читает]

## 2. Storage and Retention (Хранение и retention)

- **Where stored:** [Storage]
- **Retention policy:** [Сроки]
- **Deletion / cleanup:** [Как удаляется]

## 3. Access Model (Модель доступа)

- **Who has access:** [Роли]
- **How access is granted:** [RBAC / manual / IAM]
- **Auditability:** [Что логируется]

## 4. Logging Restrictions (Ограничения на логирование)

- **What may be logged:** [Что можно логировать]
- **What must not be logged:** [Что нельзя логировать]
- **Masking / redaction rules:** [Какие правила маскирования действуют]

## 5. Vendor and Compliance Constraints (Ограничения по вендорам и compliance)

- **External vendors:** [OpenAI / cloud / etc.]
- **Allowed / restricted data:** [Rules]
- **Compliance notes:** [152-FZ / GDPR / internal policy]

## 6. Security Controls (Контроли безопасности)

- **Encryption:** [At rest / in transit]
- **Secrets management:** [Как хранятся секреты]
- **Access control:** [RBAC / IAM / approval flow]
- **Audit / monitoring controls:** [Какие security controls обязательны]

## 7. Risks and Mitigations (Риски и митигации)

| Risk | Impact | Mitigation |
| :--- | :--- | :--- |
| [Risk] | [Impact] | [Mitigation] |
