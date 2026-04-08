#pragma once

#include <QObject>
#include <optional>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <vector>
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
  Q_PROPERTY(QVariantList tabEntries READ tabEntries NOTIFY tabsChanged)
  Q_PROPERTY(int currentTabIndex READ currentTabIndex WRITE setCurrentTabIndex NOTIFY tabsChanged)
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
  Q_PROPERTY(bool anyDirty READ anyDirty NOTIFY tabsChanged)
  Q_PROPERTY(bool saving READ saving NOTIFY savingChanged)
  Q_PROPERTY(bool generating READ generating NOTIFY generatingChanged)
  Q_PROPERTY(bool aiGenerationAvailable READ aiGenerationAvailable NOTIFY generationAvailabilityChanged)

 public:
  explicit BlockEditorViewModel(SessionViewModel* session, BlocksModel* blocks,
                                QObject* parent = nullptr);
  static BlockEditorViewModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
  static BlockEditorViewModel* instance();

  bool open() const;
  QVariantList tabEntries() const;
  int currentTabIndex() const;
  void setCurrentTabIndex(int value);
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
  bool anyDirty() const;
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
  Q_INVOKABLE void closeTab(int index);
  Q_INVOKABLE void closeAllEditors();
  Q_INVOKABLE void save();
  Q_INVOKABLE void generate();
  Q_INVOKABLE void revise();

 signals:
  void openChanged();
  void tabsChanged();
  void blockLoaded();
  void formChanged();
  void statusTextChanged();
  void savingChanged();
  void generatingChanged();
  void generationAvailabilityChanged();
  void saved();

 private:
  struct EditorTab {
    bool create_mode = false;
    QString block_id;
    QString current_version;
    QString type = QStringLiteral("domain");
    QString language = QStringLiteral("en");
    QString description;
    QString revision_comment;
    QString tags_text;
    QString defaults_text;
    QString template_text;
    QString ai_prompt_text;
    QString bump_mode = QStringLiteral("Minor");
    QString original_state_key;
  };

  void setStatusText(QString value);
  static EditorTab MakeDefaultTab();
  EditorTab* currentTab();
  const EditorTab* currentTab() const;
  QString currentStateKey(const EditorTab& tab) const;
  bool isDirty(const EditorTab& tab) const;
  QString tabTitle(const EditorTab& tab) const;
  QString tabSubtitle(const EditorTab& tab) const;
  std::optional<EditorTab> loadSelectedBlockTab();
  void activateTab(int index);
  void emitCurrentTabChanged();
  int findExistingTab(const QString& blockId, const QString& version) const;
  int findCreateTab() const;

  SessionViewModel* session_;
  BlocksModel* blocks_;
  bool saving_ = false;
  bool generating_ = false;
  QString status_text_;
  std::vector<EditorTab> tabs_;
  int current_tab_index_ = -1;
};

}  // namespace tf::gui
