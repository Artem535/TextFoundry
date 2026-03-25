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
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

 public:
  static SessionViewModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
  static SessionViewModel* instance();

  QString projectKey() const;
  QString dataPath() const;
  bool strictMode() const;
  QString statusText() const;

  void setProjectKey(const QString& value);
  void setDataPath(const QString& value);
  void setStrictMode(bool value);

  Q_INVOKABLE void reload();

  tf::Engine& engine();
  const tf::Engine& engine() const;

 signals:
  void projectKeyChanged();
  void dataPathChanged();
  void strictModeChanged();
  void statusTextChanged();
  void engineReset();

 private:
  explicit SessionViewModel(QObject* parent = nullptr);
  void rebuildEngine();
  void setStatusText(QString value);

  QString project_key_;
  QString data_path_;
  bool strict_mode_ = false;
  QString status_text_;
  std::unique_ptr<tf::Engine> engine_;
};

}  // namespace tf::gui
