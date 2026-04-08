#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqml.h>

#include "tf/engine.h"

namespace tf::gui {

class SessionViewModel;
class BlocksModel;

class BlockEditorViewModel : public QObject {
  Q_OBJECT
  QML_SINGLETON
  QML_NAMED_ELEMENT(BlockEditorVm)
  Q_PROPERTY(bool open READ open NOTIFY openChanged)
  Q_PROPERTY(bool createMode READ createMode NOTIFY blockLoaded)
  Q_PROPERTY(QString dialogTitle READ dialogTitle NOTIFY blockLoaded)
  Q_PROPERTY(QString saveButtonText READ saveButtonText NOTIFY blockLoaded)
  Q_PROPERTY(QString blockId READ blockId WRITE setBlockId NOTIFY blockLoaded)
  Q_PROPERTY(QString currentVersion READ currentVersion NOTIFY blockLoaded)
  Q_PROPERTY(QString type READ type WRITE setType NOTIFY formChanged)
  Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY formChanged)
  Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY formChanged)
  Q_PROPERTY(QString revisionComment READ revisionComment WRITE setRevisionComment NOTIFY formChanged)
  Q_PROPERTY(QString tagsText READ tagsText WRITE setTagsText NOTIFY formChanged)
  Q_PROPERTY(QString defaultsText READ defaultsText WRITE setDefaultsText NOTIFY formChanged)
  Q_PROPERTY(QString templateText READ templateText WRITE setTemplateText NOTIFY formChanged)
  Q_PROPERTY(QString aiPromptText READ aiPromptText WRITE setAiPromptText NOTIFY formChanged)
  Q_PROPERTY(QString bumpMode READ bumpMode WRITE setBumpMode NOTIFY formChanged)
  Q_PROPERTY(QStringList typeOptions READ typeOptions CONSTANT)
  Q_PROPERTY(QStringList bumpOptions READ bumpOptions CONSTANT)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
  Q_PROPERTY(bool dirty READ dirty NOTIFY formChanged)
  Q_PROPERTY(bool saving READ saving NOTIFY savingChanged)
  Q_PROPERTY(bool generating READ generating NOTIFY generatingChanged)
  Q_PROPERTY(bool aiGenerationAvailable READ aiGenerationAvailable NOTIFY generationAvailabilityChanged)

 public:
  explicit BlockEditorViewModel(SessionViewModel* session, BlocksModel* blocks,
                                QObject* parent = nullptr);
  static BlockEditorViewModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
  static BlockEditorViewModel* instance();

  bool open() const;
  bool createMode() const;
  QString dialogTitle() const;
  QString saveButtonText() const;
  QString blockId() const;
  QString currentVersion() const;
  QString type() const;
  QString language() const;
  QString description() const;
  QString revisionComment() const;
  QString tagsText() const;
  QString defaultsText() const;
  QString templateText() const;
  QString aiPromptText() const;
  QString bumpMode() const;
  QStringList typeOptions() const;
  QStringList bumpOptions() const;
  QString statusText() const;
  bool dirty() const;
  bool saving() const;
  bool generating() const;
  bool aiGenerationAvailable() const;

  void setBlockId(const QString& value);
  void setType(const QString& value);
  void setLanguage(const QString& value);
  void setDescription(const QString& value);
  void setRevisionComment(const QString& value);
  void setTagsText(const QString& value);
  void setDefaultsText(const QString& value);
  void setTemplateText(const QString& value);
  void setAiPromptText(const QString& value);
  void setBumpMode(const QString& value);

  Q_INVOKABLE void openEditor();
  Q_INVOKABLE void openCreateEditor();
  Q_INVOKABLE void closeEditor();
  Q_INVOKABLE void save();
  Q_INVOKABLE void generate();

 signals:
  void openChanged();
  void blockLoaded();
  void formChanged();
  void statusTextChanged();
  void savingChanged();
  void generatingChanged();
  void generationAvailabilityChanged();
  void saved();

 private:
  void setStatusText(QString value);
  void resetForm();
  bool loadSelectedBlock();
  QString currentStateKey() const;

  SessionViewModel* session_;
  BlocksModel* blocks_;
  bool open_ = false;
  bool create_mode_ = false;
  bool saving_ = false;
  bool generating_ = false;
  QString block_id_;
  QString current_version_;
  QString type_ = QStringLiteral("domain");
  QString language_ = QStringLiteral("en");
  QString description_;
  QString revision_comment_;
  QString tags_text_;
  QString defaults_text_;
  QString template_text_;
  QString ai_prompt_text_;
  QString bump_mode_ = QStringLiteral("Minor");
  QString status_text_;
  QString original_state_key_;
};

}  // namespace tf::gui
