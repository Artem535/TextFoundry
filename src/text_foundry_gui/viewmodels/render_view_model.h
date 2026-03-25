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
  QString statusText() const;

  void setSelectedCompositionId(const QString& value);
  void setVersionText(const QString& value);
  void setParamsText(const QString& value);

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
  void statusTextChanged();

 private:
  void setOutputText(QString value);
  void setStatusText(QString value);
  void syncCompositions();

  SessionViewModel* session_;
  QStringList composition_ids_;
  QString selected_composition_id_;
  QString version_text_;
  QString params_text_;
  QString output_text_ = QStringLiteral("Select a composition and click Render.");
  QString status_text_ = QStringLiteral("Render output actions are available after rendering.");
};

}  // namespace tf::gui
