#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqml.h>

#include <vector>

#include "tf/block_generation.h"

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

  Q_INVOKABLE void openDialog();
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
  void resetForm();
  void setStatusText(QString value);
  QString BuildCompositionPreviewId() const;
  QString BuildCompositionPreviewText() const;
  QString BuildGeneratedPreview() const;

  SessionViewModel* session_;
  BlocksModel* blocks_;
  bool open_ = false;
  bool generating_ = false;
  bool publishing_ = false;
  QString source_prompt_text_;
  QString namespace_prefix_;
  QString language_ = QStringLiteral("en");
  QString status_text_;
  std::vector<GeneratedBlockData> generated_blocks_;
};

}  // namespace tf::gui
