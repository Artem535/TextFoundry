#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqml.h>

#include "tf/engine.h"

namespace tf::gui {

class SessionViewModel;

class CompositionsViewModel : public QObject {
  Q_OBJECT
  QML_SINGLETON
  QML_NAMED_ELEMENT(CompositionsVm)
  Q_PROPERTY(QStringList compositionIds READ compositionIds NOTIFY compositionsChanged)
  Q_PROPERTY(QString selectedCompositionId READ selectedCompositionId WRITE setSelectedCompositionId NOTIFY selectedCompositionIdChanged)
  Q_PROPERTY(QString selectedVersion READ selectedVersion NOTIFY detailsChanged)
  Q_PROPERTY(QStringList selectedVersions READ selectedVersions NOTIFY detailsChanged)
  Q_PROPERTY(QStringList selectedVersionOptions READ selectedVersionOptions NOTIFY detailsChanged)
  Q_PROPERTY(QString selectedState READ selectedState NOTIFY detailsChanged)
  Q_PROPERTY(QString selectedDescription READ selectedDescription NOTIFY detailsChanged)
  Q_PROPERTY(QString selectedRevisionComment READ selectedRevisionComment NOTIFY detailsChanged)
  Q_PROPERTY(QString selectedFragmentCount READ selectedFragmentCount NOTIFY detailsChanged)
  Q_PROPERTY(QStringList selectedFragments READ selectedFragments NOTIFY detailsChanged)
  Q_PROPERTY(QString tone READ tone WRITE setTone NOTIFY normalizationChanged)
  Q_PROPERTY(QString tense READ tense WRITE setTense NOTIFY normalizationChanged)
  Q_PROPERTY(QString targetLanguage READ targetLanguage WRITE setTargetLanguage NOTIFY normalizationChanged)
  Q_PROPERTY(QString person READ person WRITE setPerson NOTIFY normalizationChanged)
  Q_PROPERTY(QString rewriteStrength READ rewriteStrength WRITE setRewriteStrength NOTIFY normalizationChanged)
  Q_PROPERTY(QString audience READ audience WRITE setAudience NOTIFY normalizationChanged)
  Q_PROPERTY(QString locale READ locale WRITE setLocale NOTIFY normalizationChanged)
  Q_PROPERTY(QString terminologyRigidity READ terminologyRigidity WRITE setTerminologyRigidity NOTIFY normalizationChanged)
  Q_PROPERTY(bool preserveFormatting READ preserveFormatting WRITE setPreserveFormatting NOTIFY normalizationChanged)
  Q_PROPERTY(bool preserveExamples READ preserveExamples WRITE setPreserveExamples NOTIFY normalizationChanged)
  Q_PROPERTY(bool normalizing READ normalizing NOTIFY normalizationChanged)
  Q_PROPERTY(bool normalizationAvailable READ normalizationAvailable NOTIFY normalizationChanged)
  Q_PROPERTY(QString normalizationStatusText READ normalizationStatusText NOTIFY normalizationChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

 public:
  explicit CompositionsViewModel(SessionViewModel* session,
                                 QObject* parent = nullptr);
  static CompositionsViewModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
  static CompositionsViewModel* instance();

  QStringList compositionIds() const;
  QString selectedCompositionId() const;
  QString selectedVersion() const;
  QStringList selectedVersions() const;
  QStringList selectedVersionOptions() const;
  QString selectedState() const;
  QString selectedDescription() const;
  QString selectedRevisionComment() const;
  QString selectedFragmentCount() const;
  QStringList selectedFragments() const;
  QString tone() const;
  QString tense() const;
  QString targetLanguage() const;
  QString person() const;
  QString rewriteStrength() const;
  QString audience() const;
  QString locale() const;
  QString terminologyRigidity() const;
  bool preserveFormatting() const;
  bool preserveExamples() const;
  bool normalizing() const;
  bool normalizationAvailable() const;
  QString normalizationStatusText() const;
  QString statusText() const;

  void setSelectedCompositionId(const QString& value);
  void setTone(const QString& value);
  void setTense(const QString& value);
  void setTargetLanguage(const QString& value);
  void setPerson(const QString& value);
  void setRewriteStrength(const QString& value);
  void setAudience(const QString& value);
  void setLocale(const QString& value);
  void setTerminologyRigidity(const QString& value);
  void setPreserveFormatting(bool value);
  void setPreserveExamples(bool value);

  Q_INVOKABLE void reload();
  Q_INVOKABLE void selectComposition(const QString& value);
  Q_INVOKABLE void selectCompositionVersion(const QString& value);
  Q_INVOKABLE void deprecateSelected();
  Q_INVOKABLE void normalizeSelected();
  Q_INVOKABLE void updateBlocksToLatest();

 signals:
  void compositionsChanged();
  void selectedCompositionIdChanged();
  void detailsChanged();
  void normalizationChanged();
  void statusTextChanged();

 private:
  void syncCompositions();
  void refreshDetails();
  void setStatusText(QString value);
  SemanticStyle currentSemanticStyle() const;

  SessionViewModel* session_;
  QStringList composition_ids_;
  QString selected_composition_id_;
  QString selected_version_;
  QStringList selected_versions_;
  QStringList selected_version_options_;
  QString selected_state_;
  QString selected_description_;
  QString selected_revision_comment_;
  QString selected_fragment_count_;
  QStringList selected_fragments_;
  QString tone_;
  QString tense_;
  QString target_language_;
  QString person_;
  QString rewrite_strength_ = QStringLiteral("light");
  QString audience_;
  QString locale_;
  QString terminology_rigidity_ = QStringLiteral("strict");
  bool preserve_formatting_ = true;
  bool preserve_examples_ = true;
  bool normalizing_ = false;
  QString normalization_status_text_ = QStringLiteral("Create a normalized composition to preserve structure and placeholders.");
  QString status_text_ = QStringLiteral("Select a composition to see details.");
};

}  // namespace tf::gui
