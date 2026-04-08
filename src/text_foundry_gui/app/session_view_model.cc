#include "app/session_view_model.h"

#include <algorithm>
#include <QCoreApplication>
#include <QEventLoop>
#include <QSettings>
#include <sago/platform_folders.h>

#include <filesystem>
#include <utility>

#if __has_include(<qtkeychain/keychain.h>)
#include <qtkeychain/keychain.h>
#elif __has_include(<qt6keychain/keychain.h>)
#include <qt6keychain/keychain.h>
#elif __has_include(<keychain.h>)
#include <keychain.h>
#else
#error "QtKeychain header not found"
#endif

#include "openai_compatible_block_generator.h"
#include "openai_compatible_normalizer.h"
#include "qt_http_transport.h"

namespace tf::gui {
namespace fs = std::filesystem;

namespace {

constexpr char kAiBaseUrlKey[] = "ai/base_url";
constexpr char kAiModelKey[] = "ai/model";
constexpr char kSecretService[] = "TextFoundry";
constexpr char kSecretKey[] = "ai_api_key";

struct SecretToolResult {
  bool ok = false;
  QString value;
  QString error;
};

SecretToolResult LookupSecret(const QString& service, const QString& key) {
  QKeychain::ReadPasswordJob job(service);
  job.setAutoDelete(false);
  job.setKey(key);

  QEventLoop loop;
  QObject::connect(&job, &QKeychain::Job::finished, &loop,
                   &QEventLoop::quit);
  job.start();
  loop.exec();

  if (job.error() == QKeychain::NoError) {
    return {.ok = true, .value = job.textData()};
  }
  if (job.error() == QKeychain::EntryNotFound) {
    return {};
  }

  return {.error = job.errorString()};
}

QString StoreSecret(const QString& service, const QString& key,
                    const QString& value) {
  QKeychain::WritePasswordJob job(service);
  job.setAutoDelete(false);
  job.setKey(key);
  job.setTextData(value);

  QEventLoop loop;
  QObject::connect(&job, &QKeychain::Job::finished, &loop,
                   &QEventLoop::quit);
  job.start();
  loop.exec();

  if (job.error() != QKeychain::NoError) {
    return job.errorString();
  }

  return {};
}

QString ClearSecret(const QString& service, const QString& key) {
  QKeychain::DeletePasswordJob job(service);
  job.setAutoDelete(false);
  job.setKey(key);

  QEventLoop loop;
  QObject::connect(&job, &QKeychain::Job::finished, &loop,
                   &QEventLoop::quit);
  job.start();
  loop.exec();

  if (job.error() != QKeychain::NoError &&
      job.error() != QKeychain::EntryNotFound) {
    return job.errorString();
  }

  return {};
}

}  // namespace

SessionViewModel::SessionViewModel(QObject* parent)
    : QObject(parent),
      project_key_("default"),
      data_path_(
          QString::fromStdString((fs::path(sago::getConfigHome()) / "TextFoundry")
                                     .string())) {
  loadPersistentSettings();
  rebuildEngine();
}

SessionViewModel* SessionViewModel::create(QQmlEngine* qmlEngine,
                                           QJSEngine* jsEngine) {
  Q_UNUSED(qmlEngine);
  Q_UNUSED(jsEngine);
  return instance();
}

SessionViewModel* SessionViewModel::instance() {
  static auto* singleton = new SessionViewModel(QCoreApplication::instance());
  return singleton;
}

QString SessionViewModel::projectKey() const { return project_key_; }

QString SessionViewModel::dataPath() const { return data_path_; }

bool SessionViewModel::strictMode() const { return strict_mode_; }

bool SessionViewModel::compositionNewlineDelimiter() const {
  return composition_newline_delimiter_;
}

int SessionViewModel::previewFontSize() const { return preview_font_size_; }

QString SessionViewModel::aiBaseUrl() const { return ai_base_url_; }

QString SessionViewModel::aiModel() const { return ai_model_; }

QString SessionViewModel::aiApiKey() const { return ai_api_key_; }

bool SessionViewModel::aiGenerationEnabled() const {
  return engine_ != nullptr && engine_->HasBlockGenerator();
}

QString SessionViewModel::statusText() const { return status_text_; }

void SessionViewModel::loadPersistentSettings() {
  QSettings settings;
  ai_base_url_ = settings.value(QString::fromLatin1(kAiBaseUrlKey), ai_base_url_)
                     .toString();
  ai_model_ =
      settings.value(QString::fromLatin1(kAiModelKey), ai_model_).toString();

  const SecretToolResult secret =
      LookupSecret(QString::fromLatin1(kSecretService),
                   QString::fromLatin1(kSecretKey));
  if (secret.ok) {
    ai_api_key_ = secret.value;
  }
}

void SessionViewModel::setProjectKey(const QString& value) {
  if (project_key_ == value) return;
  project_key_ = value;
  emit projectKeyChanged();
  rebuildEngine();
}

void SessionViewModel::setDataPath(const QString& value) {
  if (data_path_ == value) return;
  data_path_ = value;
  emit dataPathChanged();
  rebuildEngine();
}

void SessionViewModel::setStrictMode(const bool value) {
  if (strict_mode_ == value) return;
  strict_mode_ = value;
  emit strictModeChanged();
  rebuildEngine();
}

void SessionViewModel::setCompositionNewlineDelimiter(const bool value) {
  if (composition_newline_delimiter_ == value) return;
  composition_newline_delimiter_ = value;
  emit compositionNewlineDelimiterChanged();
  setStatusText(QStringLiteral("Composition creation setting updated."));
}

void SessionViewModel::setPreviewFontSize(const int value) {
  const int clamped = std::clamp(value, 11, 24);
  if (preview_font_size_ == clamped) return;
  preview_font_size_ = clamped;
  emit previewFontSizeChanged();
  setStatusText(QString("Preview font size set to %1.").arg(preview_font_size_));
}

void SessionViewModel::setAiBaseUrl(const QString& value) {
  if (ai_base_url_ == value) return;
  ai_base_url_ = value;
  QSettings().setValue(QString::fromLatin1(kAiBaseUrlKey), ai_base_url_);
  emit aiBaseUrlChanged();
  rebuildEngine();
}

void SessionViewModel::setAiModel(const QString& value) {
  if (ai_model_ == value) return;
  ai_model_ = value;
  QSettings().setValue(QString::fromLatin1(kAiModelKey), ai_model_);
  emit aiModelChanged();
  rebuildEngine();
}

void SessionViewModel::setAiApiKey(const QString& value) {
  if (ai_api_key_ == value) return;
  ai_api_key_ = value;
  const QString error =
      ai_api_key_.trimmed().isEmpty()
          ? ClearSecret(QString::fromLatin1(kSecretService),
                        QString::fromLatin1(kSecretKey))
          : StoreSecret(QString::fromLatin1(kSecretService),
                        QString::fromLatin1(kSecretKey), ai_api_key_);
  emit aiApiKeyChanged();
  rebuildEngine();
  if (!error.isEmpty()) {
    setStatusText(QStringLiteral(
                      "Engine ready, but API key could not be persisted: %1")
                      .arg(error));
  }
}

void SessionViewModel::reload() { rebuildEngine(); }

void SessionViewModel::publishStatus(const QString& value) {
  setStatusText(value);
}

tf::Engine& SessionViewModel::engine() { return *engine_; }

const tf::Engine& SessionViewModel::engine() const { return *engine_; }

void SessionViewModel::rebuildEngine() {
  tf::EngineConfig config{
      .ProjectKey = project_key_.toStdString(),
      .strict_mode = strict_mode_,
      .default_data_path = data_path_.toStdString(),
  };
  // ObjectBox allows only one open store per path in this process.
  // Destroy the previous engine before constructing the next one.
  engine_.reset();
  engine_ = std::make_unique<tf::Engine>(std::move(config));
  if (!ai_base_url_.trimmed().isEmpty() && !ai_model_.trimmed().isEmpty() &&
      !ai_api_key_.trimmed().isEmpty()) {
    auto transport = std::make_shared<tf::ai::QtHttpTransport>();
    auto generator = std::make_shared<tf::ai::OpenAiCompatibleBlockGenerator>(
        tf::ai::OpenAiCompatibleConfig{
            .base_url = ai_base_url_.trimmed().toStdString(),
            .model = ai_model_.trimmed().toStdString(),
            .api_key = ai_api_key_.trimmed().toStdString(),
        },
        transport);
    auto normalizer = std::make_shared<tf::ai::OpenAiCompatibleNormalizer>(
        tf::ai::OpenAiCompatibleConfig{
            .base_url = ai_base_url_.trimmed().toStdString(),
            .model = ai_model_.trimmed().toStdString(),
            .api_key = ai_api_key_.trimmed().toStdString(),
        },
        transport);
    engine_->SetBlockGenerator(std::move(generator));
    engine_->SetNormalizer(normalizer);
    engine_->SetBlockNormalizer(std::move(normalizer));
  }
  setStatusText(
      QString("Engine ready for project '%1'%2")
          .arg(project_key_.isEmpty() ? QStringLiteral("default") : project_key_,
               engine_->HasBlockGenerator() ? QStringLiteral(" with AI block generation")
                                            : QString()));
  emit engineReset();
}

void SessionViewModel::setStatusText(QString value) {
  if (status_text_ == value) return;
  status_text_ = std::move(value);
  emit statusTextChanged();
}

}  // namespace tf::gui
