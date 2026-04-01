#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
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
  Q_PROPERTY(QStringList filteredCompositionIds READ filteredCompositionIds NOTIFY compositionsChanged)
  Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY compositionsChanged)
  Q_PROPERTY(QString selectedCompositionId READ selectedCompositionId WRITE setSelectedCompositionId NOTIFY selectedCompositionIdChanged)
  Q_PROPERTY(QVariantList versionEntries READ versionEntries NOTIFY selectedCompositionIdChanged)
  Q_PROPERTY(QString versionText READ versionText WRITE setVersionText NOTIFY versionTextChanged)
  Q_PROPERTY(QString paramsText READ paramsText WRITE setParamsText NOTIFY paramsTextChanged)
  Q_PROPERTY(QString outputText READ outputText NOTIFY outputTextChanged)
  Q_PROPERTY(QString rawOutputText READ rawOutputText NOTIFY rawOutputTextChanged)
  Q_PROPERTY(QString normalizedOutputText READ normalizedOutputText NOTIFY normalizedOutputTextChanged)
  Q_PROPERTY(QString previewMode READ previewMode WRITE setPreviewMode NOTIFY previewModeChanged)
  Q_PROPERTY(QStringList previewModes READ previewModes CONSTANT)
  Q_PROPERTY(QString displayedOutputText READ displayedOutputText NOTIFY displayedOutputTextChanged)
  Q_PROPERTY(QString tone READ tone WRITE setTone NOTIFY semanticStyleChanged)
  Q_PROPERTY(QString tense READ tense WRITE setTense NOTIFY semanticStyleChanged)
  Q_PROPERTY(QString targetLanguage READ targetLanguage WRITE setTargetLanguage NOTIFY semanticStyleChanged)
  Q_PROPERTY(QString person READ person WRITE setPerson NOTIFY semanticStyleChanged)
  Q_PROPERTY(QString rewriteStrength READ rewriteStrength WRITE setRewriteStrength NOTIFY semanticStyleChanged)
  Q_PROPERTY(QString audience READ audience WRITE setAudience NOTIFY semanticStyleChanged)
  Q_PROPERTY(QString locale READ locale WRITE setLocale NOTIFY semanticStyleChanged)
  Q_PROPERTY(QString terminologyRigidity READ terminologyRigidity WRITE setTerminologyRigidity NOTIFY semanticStyleChanged)
  Q_PROPERTY(bool preserveFormatting READ preserveFormatting WRITE setPreserveFormatting NOTIFY semanticStyleChanged)
  Q_PROPERTY(bool preserveExamples READ preserveExamples WRITE setPreserveExamples NOTIFY semanticStyleChanged)
  Q_PROPERTY(bool normalizing READ normalizing NOTIFY normalizingChanged)
  Q_PROPERTY(bool normalizationAvailable READ normalizationAvailable NOTIFY normalizationAvailabilityChanged)
  Q_PROPERTY(bool normalizationStale READ normalizationStale NOTIFY normalizationStateChanged)
  Q_PROPERTY(QString normalizationStatusText READ normalizationStatusText NOTIFY normalizationStateChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

 public:
  explicit RenderViewModel(SessionViewModel* session, QObject* parent = nullptr);
  static RenderViewModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
  static RenderViewModel* instance();

  QStringList compositionIds() const;
  QStringList filteredCompositionIds() const;
  QString searchText() const;
  QString selectedCompositionId() const;
  QVariantList versionEntries() const;
  QString versionText() const;
  QString paramsText() const;
  QString outputText() const;
  QString rawOutputText() const;
  QString normalizedOutputText() const;
  QString previewMode() const;
  QStringList previewModes() const;
  QString displayedOutputText() const;
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
  bool normalizationStale() const;
  QString normalizationStatusText() const;
  QString statusText() const;

  void setSelectedCompositionId(const QString& value);
  void setSearchText(const QString& value);
  void setVersionText(const QString& value);
  void setParamsText(const QString& value);
  void setPreviewMode(const QString& value);
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
  Q_INVOKABLE void render();
  Q_INVOKABLE void normalize();
  Q_INVOKABLE void renormalize();
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
  void normalizedOutputTextChanged();
  void previewModeChanged();
  void displayedOutputTextChanged();
  void semanticStyleChanged();
  void normalizingChanged();
  void normalizationAvailabilityChanged();
  void normalizationStateChanged();
  void statusTextChanged();

 private:
  void setOutputText(QString value);
  void setRawOutputText(QString value);
  void setNormalizedOutputText(QString value);
  void setStatusText(QString value);
  void syncCompositions();
  void refreshFilteredCompositions();
  Result<QString> buildRawOutput() const;
  void refreshRawPreview();
  void updateNormalizationState(bool raw_changed);
  SemanticStyle currentSemanticStyle() const;
  QString currentRawHash() const;
  void runNormalization(bool force);

  SessionViewModel* session_;
  QStringList composition_ids_;
  QStringList filtered_composition_ids_;
  QString search_text_;
  QString selected_composition_id_;
  QVariantList version_entries_;
  QString version_text_;
  QString params_text_;
  QString output_text_ = QStringLiteral("Select a composition and click Render.");
  QString raw_output_text_ = QStringLiteral("Select a composition to preview raw output.");
  QString normalized_output_text_ = QStringLiteral("Click Normalize to rewrite the current raw text snapshot.");
  QString preview_mode_ = QStringLiteral("Raw");
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
  bool normalization_stale_ = false;
  QString normalization_status_text_ = QStringLiteral("Normalization here is a text rewrite utility over the current raw snapshot.");
  QString last_normalized_raw_hash_;
  QString status_text_ = QStringLiteral("Render output actions are available after rendering.");
};

}  // namespace tf::gui
