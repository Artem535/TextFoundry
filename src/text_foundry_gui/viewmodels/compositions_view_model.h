#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqml.h>

namespace tf::gui {

class SessionViewModel;

class CompositionsViewModel : public QObject {
  Q_OBJECT
  QML_SINGLETON
  QML_NAMED_ELEMENT(CompositionsVm)
  Q_PROPERTY(QStringList compositionIds READ compositionIds NOTIFY compositionsChanged)
  Q_PROPERTY(QString selectedCompositionId READ selectedCompositionId WRITE setSelectedCompositionId NOTIFY selectedCompositionIdChanged)
  Q_PROPERTY(QString selectedVersion READ selectedVersion NOTIFY detailsChanged)
  Q_PROPERTY(QString selectedState READ selectedState NOTIFY detailsChanged)
  Q_PROPERTY(QString selectedDescription READ selectedDescription NOTIFY detailsChanged)
  Q_PROPERTY(QString selectedFragmentCount READ selectedFragmentCount NOTIFY detailsChanged)
  Q_PROPERTY(QStringList selectedFragments READ selectedFragments NOTIFY detailsChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

 public:
  explicit CompositionsViewModel(SessionViewModel* session,
                                 QObject* parent = nullptr);
  static CompositionsViewModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
  static CompositionsViewModel* instance();

  QStringList compositionIds() const;
  QString selectedCompositionId() const;
  QString selectedVersion() const;
  QString selectedState() const;
  QString selectedDescription() const;
  QString selectedFragmentCount() const;
  QStringList selectedFragments() const;
  QString statusText() const;

  void setSelectedCompositionId(const QString& value);

  Q_INVOKABLE void reload();
  Q_INVOKABLE void selectComposition(const QString& value);

 signals:
  void compositionsChanged();
  void selectedCompositionIdChanged();
  void detailsChanged();
  void statusTextChanged();

 private:
  void syncCompositions();
  void refreshDetails();
  void setStatusText(QString value);

  SessionViewModel* session_;
  QStringList composition_ids_;
  QString selected_composition_id_;
  QString selected_version_;
  QString selected_state_;
  QString selected_description_;
  QString selected_fragment_count_;
  QStringList selected_fragments_;
  QString status_text_ = QStringLiteral("Select a composition to see details.");
};

}  // namespace tf::gui
