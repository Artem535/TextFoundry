#include <tf/engine.h>

#include <fmt/format.h>
#include <iostream>

using namespace tf;

void print_render_result(const Result<RenderResult>& result) {
  if (result.HasError()) {
    std::cerr << fmt::format("❌ Ошибка [{}]: {}\n",
                             static_cast<int>(result.error().code),
                             result.error().message);
    return;
  }
  const auto& rr = result.value();
  std::cout << fmt::format("✅ Успех | Композиция: {} v{}.{} | Блоков: {}\n",
                           rr.compositionId, rr.compositionVersion.major,
                           rr.compositionVersion.minor, rr.blocksUsed.size());
  std::cout << fmt::format("Текст ({} символов):\n{}\n", rr.text.length(),
                           rr.text);
  std::cout << "------------------------------------------------\n\n";
}

int main() {
  try {
    // Инициализация
    EngineConfig config{.ProjectKey = "doc-system",
                        .strict_mode = true,
                        .default_data_path = "./tf_data"};
    Engine engine(config);

    std::cout << "=== TextFoundry: Пример с явным версионированием ===\n\n";

    // === 1. Создание и публикация блоков ===
    std::cout << "📦 Шаг 1. Создание и публикация блоков...\n";

    // Блок 1: Заголовок
    auto header_draft = BlockDraftBuilder("doc.header")
                            .WithType(BlockType::Meta)
                            .WithTemplate(Template(R"(# {{title}}
**Версия:** {{version}} | **Автор:** {{author}} | **Дата:** {{date}})"))
                            .WithDefault("author", "API Team")
                            .WithDefault("date", "2026-02-02")
                            .WithDescription("Заголовок документа")
                            .build();

    auto header_pub =
        engine.PublishBlock(std::move(header_draft), Version{1, 0});
    if (!header_pub.HasValue()) {
      std::cerr << "Ошибка публикации header: " << header_pub.error().message
                << "\n";
      return 1;
    }

    // Получаем версию для использования в композиции
    Version header_version = header_pub.value().version();
    std::cout << fmt::format("   ✅ doc.header опубликован v{}.{}\n",
                             header_version.major, header_version.minor);

    // Блок 2: Описание endpoint'а
    auto endpoint_draft = BlockDraftBuilder("api.endpoint")
                              .WithType(BlockType::Domain)
                              .WithTemplate(Template(R"(## {{method}} {{path}}

{{description}}

**Параметры:**
- `{{param}}` ({{type}}): {{desc}})"))
                              .WithDefault("type", "string")
                              .WithDefault("desc", "Описание параметра")
                              .build();

    auto endpoint_pub =
        engine.PublishBlock(std::move(endpoint_draft), Version{1, 0});
    Version endpoint_version = endpoint_pub.value().version();
    std::cout << fmt::format("   ✅ api.endpoint опубликован v{}.{}\n",
                             endpoint_version.major, endpoint_version.minor);

    // Блок 3: Ограничения (rate limit)
    auto limit_draft = BlockDraftBuilder("api.constraint")
                           .WithType(BlockType::Constraint)
                           .WithTemplate(Template(
                               R"(> ⚠️ **Ограничение:** {{limit}} запросов/мин
> Текущий план: {{plan}})"))
                           .WithDefault("limit", "100")
                           .WithDefault("plan", "basic")
                           .build();

    auto limit_pub = engine.PublishBlock(std::move(limit_draft), Version{1, 0});
    Version limit_version = limit_pub.value().version();
    std::cout << fmt::format("   ✅ api.constraint опубликован v{}.{}\n",
                             limit_version.major, limit_version.minor);

    // === 2. Создание композиции с ЯВНЫМИ версиями ===
    std::cout << "\n📝 Шаг 2. Создание композиции...\n";

    // ✅ Используем только AddBlockRef с BlockRef, содержащим конкретную
    // версию
    auto comp_draft =
        CompositionDraftBuilder("docs.users.create")
            .WithProjectKey("api-docs")
            .WithDescription("Документация создания пользователя")

            // Заголовок с явной версией v1.0
            .AddBlockRef(
                BlockRef("doc.header", header_version,
                         {{"title", "POST /users - Создание пользователя"},
                          {"version", "1.2.0"}}))
            .AddSeparator(SeparatorType::Paragraph)

            // Описание endpoint'а с явной версией
            .AddBlockRef(BlockRef(
                "api.endpoint", endpoint_version,
                {{"method", "POST"},
                 {"path", "/api/v1/users"},
                 {"description", "Создает нового пользователя в системе"},
                 {"param", "email"},
                 {"type", "string (email)"},
                 {"desc", "Email пользователя (должен быть уникальным)"}}))
            .AddSeparator(SeparatorType::Newline)

            // Статический контент (не требует версии)
            .AddStaticText(R"(## Дополнительная информация

При создании пользователя:
- Отправляется welcome-email
- Создается профиль по умолчанию
- Назначается роль "user"

)")

            // Ограничение с явной версией и локальными параметрами
            .AddBlockRef(BlockRef("api.constraint", limit_version,
                                  {{"limit", "500"}, {"plan", "premium"}}))
            .AddSeparator(SeparatorType::Paragraph)

            // Пример кода (статический)
            .AddStaticText(R"(## Пример запроса

```bash
curl -X POST https://api.example.com/v1/users \
  -H "Authorization: Bearer TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"email": "user@example.com"}'
```)")
            .AddSeparator(SeparatorType::Newline)

            // Подвал (статический)
            .AddStaticText("---\n*Документация актуальна на 2026 год*")
            .build();

    // === 3. Публикация композиции ===
    std::cout << "📌 Шаг 3. Публикация композиции...\n";

    auto comp_pub =
        engine.PublishComposition(std::move(comp_draft), Version{1, 0});
    if (!comp_pub.HasValue()) {
      std::cerr << "Ошибка публикации композиции: " << comp_pub.error().message
                << "\n";
      return 1;
    }

    std::cout << fmt::format(
        "   ✅ Композиция {} опубликована v{}.{}\n", comp_pub.value().id(),
        comp_pub.value().version().major, comp_pub.value().version().minor);

    // === 4. Рендеринг ===
    std::cout << "\n🎨 Шаг 4. Рендеринг композиции...\n";

    // Базовый рендеринг
    auto result1 = engine.Render("docs.users.create", Version{1, 0});
    print_render_result(result1);

    // === 5. Демонстрация иерархии параметров ===
    std::cout << "🔄 Шаг 5. Рендеринг с runtime-переопределением...\n";

    // Runtime параметры имеют высший приоритет!
    RenderContext ctx;
    ctx.WithParam("author",
                  "Старший разработчик")  // Переопределяет дефолт блока
        .WithParam("plan", "enterprise")  // Переопределяет local params
        .WithParam("limit", "1000");      // Переопределяет local params

    auto result2 = engine.Render("docs.users.create", Version{1, 0}, ctx);
    print_render_result(result2);

    // === 6. Обновление блока (новая версия) ===
    std::cout << "📈 Шаг 6. Обновление блока до v2.0...\n";

    auto header_v2 = BlockDraftBuilder("doc.header")
                         .WithTemplate(Template(R"(# {{title}}
**Версия:** {{version}} | **Автор:** {{author}} | **Статус:** {{status}})"))
                         .WithDefault("author", "API Team")
                         .WithDefault("status", "stable")
                         .build();

    auto header_pub_v2 =
        engine.PublishBlock(std::move(header_v2), Version{2, 0});
    std::cout << fmt::format("   ✅ doc.header v2.0 создан\n");

    // Старая композиция все еще использует v1.0 (воспроизводимость!)
    std::cout << "   ℹ️ Старая композиция все еще ссылается на v1.0\n";

    // Создаем новую композицию с v2.0
    auto new_comp =
        CompositionDraftBuilder("docs.users.create.v2")
            .WithProjectKey("api-docs")
            .AddBlockRef(BlockRef("doc.header", Version{2, 0},
                                  {// Явно v2.0!
                                   {"title", "POST /users (Обновлено)"},
                                   {"version", "2.0"},
                                   {"status", "beta"}}))
            .AddSeparator(SeparatorType::Paragraph)
            .AddBlockRef(BlockRef(
                "api.endpoint", endpoint_version,
                {{"method", "POST"},
                 {"path", "/api/v1/users"},
                 {"description", "Создает пользователя (обновленная логика)"}}))
            .build();

    engine.PublishComposition(std::move(new_comp), Version{2, 0});

    auto result3 = engine.Render("docs.users.create.v2", Version{2, 0});
    print_render_result(result3);

    // === 7. Демонстрация ошибки (отсутствующий параметр) ===
    std::cout << "⚠️ Шаг 7. Демонстрация обработки ошибок...\n";

    auto strict_block =
        BlockDraftBuilder("strict.test")
            .WithTemplate(Template("{{mandatory_field}}"))  // Нет дефолта!
            .build();
    engine.PublishBlock(std::move(strict_block), Version{1, 0});

    auto bad_comp =
        CompositionDraftBuilder("bad.composition")
            .AddBlockRef(BlockRef(
                "strict.test", Version{1, 0}))  // Не передали mandatory_field
            .build();
    engine.PublishComposition(std::move(bad_comp), Version{1, 0});

    // strict_mode=true в конфиге - ошибка при отсутствии параметра
    auto bad_result = engine.Render("bad.composition", Version{1, 0});
    if (bad_result.HasError()) {
      std::cout << fmt::format("   ⚠️ Ожидаемая ошибка: {}",
                               bad_result.error().message);
    }

    std::cout << "\n✅ Демонстрация завершена!\n";
    return 0;

  } catch (const std::exception& e) {
    std::cerr << "Критическая ошибка: " << e.what() << "\n";
    return 1;
  }
}
