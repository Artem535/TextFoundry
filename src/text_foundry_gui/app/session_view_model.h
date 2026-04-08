#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqml.h>

#include <memory>

#include "tf/engine.h"

namespace tf::gui {

class SessionViewModel : public QObject {
  Q_OBJECT
  QML_SINGLETON
  QML_NAMED_ELEMENT(SessionVm)
  Q_PROPERTY(QString projectKey READ projectKey WRITE setProjectKey NOTIFY projectKeyChanged)
  Q_PROPERTY(QString dataPath READ dataPath WRITE setDataPath NOTIFY dataPathChanged)
  Q_PROPERTY(bool strictMode READ strictMode WRITE setStrictMode NOTIFY strictModeChanged)
  Q_PROPERTY(bool compositionNewlineDelimiter READ compositionNewlineDelimiter WRITE setCompositionNewlineDelimiter NOTIFY compositionNewlineDelimiterChanged)
  Q_PROPERTY(int previewFontSize READ previewFontSize WRITE setPreviewFontSize NOTIFY previewFontSizeChanged)
  Q_PROPERTY(QString aiBaseUrl READ aiBaseUrl WRITE setAiBaseUrl NOTIFY aiBaseUrlChanged)
  Q_PROPERTY(QString aiModel READ aiModel WRITE setAiModel NOTIFY aiModelChanged)
  Q_PROPERTY(QString aiApiKey READ aiApiKey WRITE setAiApiKey NOTIFY aiApiKeyChanged)
  Q_PROPERTY(bool aiGenerationEnabled READ aiGenerationEnabled NOTIFY engineReset)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

 public:
  static SessionViewModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
  static SessionViewModel* instance();

  QString projectKey() const;
  QString dataPath() const;
  bool strictMode() const;
  bool compositionNewlineDelimiter() const;
  int previewFontSize() const;
  QString aiBaseUrl() const;
  QString aiModel() const;
  QString aiApiKey() const;
  bool aiGenerationEnabled() const;
  QString statusText() const;

  void setProjectKey(const QString& value);
  void setDataPath(const QString& value);
  void setStrictMode(bool value);
  void setCompositionNewlineDelimiter(bool value);
  void setPreviewFontSize(int value);
  void setAiBaseUrl(const QString& value);
  void setAiModel(const QString& value);
  void setAiApiKey(const QString& value);

  Q_INVOKABLE void reload();
  Q_INVOKABLE void publishStatus(const QString& value);

  tf::Engine& engine();
  const tf::Engine& engine() const;

 signals:
  void projectKeyChanged();
  void dataPathChanged();
  void strictModeChanged();
  void compositionNewlineDelimiterChanged();
  void previewFontSizeChanged();
  void aiBaseUrlChanged();
  void aiModelChanged();
  void aiApiKeyChanged();
  void statusTextChanged();
  void engineReset();

 private:
  explicit SessionViewModel(QObject* parent = nullptr);
  void loadPersistentSettings();
  void rebuildEngine();
  void setStatusText(QString value);

  QString project_key_;
  QString data_path_;
  bool strict_mode_ = false;
  bool composition_newline_delimiter_ = true;
  int preview_font_size_ = 15;
  QString ai_base_url_ = QStringLiteral("https://api.openai.com/v1");
  QString ai_model_ = QStringLiteral("gpt-4.1-mini");
  QString ai_api_key_;
  QString status_text_;
  std::unique_ptr<tf::Engine> engine_;
};

}  // namespace tf::gui
