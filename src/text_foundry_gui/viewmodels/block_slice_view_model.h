#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqml.h>

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "tf/block_generation.h"
#include "tf/composition.h"

namespace tf::gui {

class SessionViewModel;
class BlocksModel;

class BlockSliceViewModel : public QObject {
  Q_OBJECT
  QML_SINGLETON
  QML_NAMED_ELEMENT(BlockSliceVm)
  Q_PROPERTY(bool open READ open NOTIFY openChanged)
  Q_PROPERTY(QString sourcePromptText READ sourcePromptText WRITE setSourcePromptText NOTIFY formChanged)
  Q_PROPERTY(QString namespacePrefix READ namespacePrefix WRITE setNamespacePrefix NOTIFY formChanged)
  Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY formChanged)
  Q_PROPERTY(QString revisionComment READ revisionComment WRITE setRevisionComment NOTIFY formChanged)
  Q_PROPERTY(QString dialogTitle READ dialogTitle NOTIFY formChanged)
  Q_PROPERTY(QString publishButtonText READ publishButtonText NOTIFY formChanged)
  Q_PROPERTY(bool updateMode READ updateMode NOTIFY formChanged)
  Q_PROPERTY(int preserveStructurePercent READ preserveStructurePercent WRITE setPreserveStructurePercent NOTIFY formChanged)
  Q_PROPERTY(QString targetCompositionId READ targetCompositionId NOTIFY formChanged)
  Q_PROPERTY(QString targetCompositionVersion READ targetCompositionVersion NOTIFY formChanged)
  Q_PROPERTY(QString compositionPreviewId READ compositionPreviewId NOTIFY generatedChanged)
  Q_PROPERTY(QString compositionPreviewText READ compositionPreviewText NOTIFY generatedChanged)
  Q_PROPERTY(QString generatedPreviewText READ generatedPreviewText NOTIFY generatedChanged)
  Q_PROPERTY(int generatedCount READ generatedCount NOTIFY generatedChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
  Q_PROPERTY(bool generating READ generating NOTIFY generatingChanged)
  Q_PROPERTY(bool publishing READ publishing NOTIFY publishingChanged)
  Q_PROPERTY(bool aiGenerationAvailable READ aiGenerationAvailable NOTIFY generationAvailabilityChanged)

 public:
  explicit BlockSliceViewModel(SessionViewModel* session, BlocksModel* blocks,
                               QObject* parent = nullptr);
  static BlockSliceViewModel* create(QQmlEngine* qmlEngine,
                                     QJSEngine* jsEngine);
  static BlockSliceViewModel* instance();

  bool open() const;
  QString sourcePromptText() const;
  QString namespacePrefix() const;
  QString language() const;
  QString revisionComment() const;
  QString dialogTitle() const;
  QString publishButtonText() const;
  bool updateMode() const;
  int preserveStructurePercent() const;
  QString targetCompositionId() const;
  QString targetCompositionVersion() const;
  QString compositionPreviewId() const;
  QString compositionPreviewText() const;
  QString generatedPreviewText() const;
  int generatedCount() const;
  QString statusText() const;
  bool generating() const;
  bool publishing() const;
  bool aiGenerationAvailable() const;

  void setSourcePromptText(const QString& value);
  void setNamespacePrefix(const QString& value);
  void setLanguage(const QString& value);
  void setRevisionComment(const QString& value);
  void setPreserveStructurePercent(int value);

  Q_INVOKABLE void openDialog();
  Q_INVOKABLE void openUpdateDialog(const QString& compositionId,
                                    const QString& versionText);
  Q_INVOKABLE void closeDialog();
  Q_INVOKABLE void generate();
  Q_INVOKABLE void publishAll();

 signals:
  void openChanged();
  void formChanged();
  void generatedChanged();
  void statusTextChanged();
  void generatingChanged();
  void publishingChanged();
  void generationAvailabilityChanged();
  void published();

 private:
  void invalidateGeneratedBlocks();
  void resetForm();
  void setStatusText(QString value);
  std::optional<Composition> loadTargetComposition(QString* error_message) const;
  std::vector<BlockId> reusableBlockIdsForTarget() const;
  std::vector<std::string> reusableBlockSummariesForTarget() const;
  std::vector<BlockId> reusableBlockIdsForNamespace() const;
  std::vector<BlockId> reusableBlockIdsForCurrentOperation() const;
  std::vector<BlockId> disallowedExistingBlockIds() const;
  std::optional<QString> validateGeneratedBlocks(
      const std::vector<GeneratedBlockData>& blocks) const;
  QString defaultRevisionComment() const;
  QString reusableCompositionIdForCurrentOperation() const;
  QString BuildCompositionPreviewId() const;
  QString BuildCompositionPreviewText() const;
  QString BuildGeneratedPreview() const;

  enum class Mode { CreateBatch, UpdateComposition };

  SessionViewModel* session_;
  BlocksModel* blocks_;
  Mode mode_ = Mode::CreateBatch;
  bool open_ = false;
  bool generating_ = false;
  bool publishing_ = false;
  QString source_prompt_text_;
  QString namespace_prefix_;
  QString language_ = QStringLiteral("en");
  QString revision_comment_;
  int preserve_structure_percent_ = 70;
  QString status_text_;
  QString target_composition_id_;
  QString target_composition_version_;
  std::vector<GeneratedBlockData> generated_blocks_;
};

}  // namespace tf::gui
