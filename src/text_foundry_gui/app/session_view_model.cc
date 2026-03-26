#include "app/session_view_model.h"

#include <algorithm>
#include <QCoreApplication>
#include <sago/platform_folders.h>

#include <filesystem>
#include <utility>

namespace tf::gui {
namespace fs = std::filesystem;

SessionViewModel::SessionViewModel(QObject* parent)
    : QObject(parent),
      project_key_("default"),
      data_path_(
          QString::fromStdString((fs::path(sago::getConfigHome()) / "TextFoundry")
                                     .string())) {
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

QString SessionViewModel::statusText() const { return status_text_; }

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

void SessionViewModel::reload() { rebuildEngine(); }

tf::Engine& SessionViewModel::engine() { return *engine_; }

const tf::Engine& SessionViewModel::engine() const { return *engine_; }

void SessionViewModel::rebuildEngine() {
  tf::EngineConfig config{
      .ProjectKey = project_key_.toStdString(),
      .strict_mode = strict_mode_,
      .default_data_path = data_path_.toStdString(),
  };
  engine_ = std::make_unique<tf::Engine>(std::move(config));
  setStatusText(
      QString("Engine ready for project '%1'").arg(project_key_.isEmpty()
                                                       ? QStringLiteral("default")
                                                       : project_key_));
  emit engineReset();
}

void SessionViewModel::setStatusText(QString value) {
  if (status_text_ == value) return;
  status_text_ = std::move(value);
  emit statusTextChanged();
}

}  // namespace tf::gui
