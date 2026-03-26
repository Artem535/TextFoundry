#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqml.h>

#include "tf/composition.h"
#include "tf/engine.h"

namespace tf::gui {

class SessionViewModel;

class RenderViewModel : public QObject {
  Q_OBJECT
  QML_SINGLETON
  QML_NAMED_ELEMENT(RenderVm)
  Q_PROPERTY(QStringList compositionIds READ compositionIds NOTIFY compositionsChanged)
  Q_PROPERTY(QString selectedCompositionId READ selectedCompositionId WRITE setSelectedCompositionId NOTIFY selectedCompositionIdChanged)
  Q_PROPERTY(QString versionText READ versionText WRITE setVersionText NOTIFY versionTextChanged)
  Q_PROPERTY(QString paramsText READ paramsText WRITE setParamsText NOTIFY paramsTextChanged)
  Q_PROPERTY(QString outputText READ outputText NOTIFY outputTextChanged)
  Q_PROPERTY(QString rawOutputText READ rawOutputText NOTIFY rawOutputTextChanged)
  Q_PROPERTY(QString previewMode READ previewMode WRITE setPreviewMode NOTIFY previewModeChanged)
  Q_PROPERTY(QStringList previewModes READ previewModes CONSTANT)
  Q_PROPERTY(QString displayedOutputText READ displayedOutputText NOTIFY displayedOutputTextChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

 public:
  explicit RenderViewModel(SessionViewModel* session, QObject* parent = nullptr);
  static RenderViewModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
  static RenderViewModel* instance();

  QStringList compositionIds() const;
  QString selectedCompositionId() const;
  QString versionText() const;
  QString paramsText() const;
  QString outputText() const;
  QString rawOutputText() const;
  QString previewMode() const;
  QStringList previewModes() const;
  QString displayedOutputText() const;
  QString statusText() const;

  void setSelectedCompositionId(const QString& value);
  void setVersionText(const QString& value);
  void setParamsText(const QString& value);
  void setPreviewMode(const QString& value);

  Q_INVOKABLE void reload();
  Q_INVOKABLE void render();
  Q_INVOKABLE void copyRender();
  Q_INVOKABLE void copyRaw();
  Q_INVOKABLE void clear();

 signals:
  void compositionsChanged();
  void selectedCompositionIdChanged();
  void versionTextChanged();
  void paramsTextChanged();
  void outputTextChanged();
  void rawOutputTextChanged();
  void previewModeChanged();
  void displayedOutputTextChanged();
  void statusTextChanged();

 private:
  void setOutputText(QString value);
  void setRawOutputText(QString value);
  void setStatusText(QString value);
  void syncCompositions();
  Result<QString> buildRawOutput() const;
  void refreshRawPreview();

  SessionViewModel* session_;
  QStringList composition_ids_;
  QString selected_composition_id_;
  QString version_text_;
  QString params_text_;
  QString output_text_ = QStringLiteral("Select a composition and click Render.");
  QString raw_output_text_ = QStringLiteral("Select a composition to preview raw output.");
  QString preview_mode_ = QStringLiteral("Raw");
  QString status_text_ = QStringLiteral("Render output actions are available after rendering.");
};

}  // namespace tf::gui
