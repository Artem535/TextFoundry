#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqml.h>

#include "tf/engine.h"

namespace tf::gui {

class SessionViewModel;
class CompositionsViewModel;

class CompositionFragmentsModel : public QAbstractListModel {
  Q_OBJECT

 public:
  struct FragmentSpec {
    enum class Kind { BlockRef, StaticText, Separator };

    Kind kind = Kind::StaticText;
    QString block_id;
    QString version;
    QString params;
    QString text;
    QString separator;
  };

  enum Role {
    KindRole = Qt::UserRole + 1,
    KindLabelRole,
    TitleRole,
    MetaRole,
    BodyRole,
  };

  explicit CompositionFragmentsModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = {}) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  [[nodiscard]] const std::vector<FragmentSpec>& fragments() const noexcept;
  [[nodiscard]] const FragmentSpec* fragmentAt(int index) const noexcept;

  void setFragments(std::vector<FragmentSpec> fragments);
  void appendFragment(FragmentSpec fragment);
  void insertFragment(int index, FragmentSpec fragment);
  void updateFragment(int index, FragmentSpec fragment);
  void removeFragment(int index);
  void moveFragment(int from, int to);

 private:
  [[nodiscard]] QVariant dataForFragment(const FragmentSpec& fragment,
                                         int role) const;

  std::vector<FragmentSpec> fragments_;
};

class CompositionEditorViewModel : public QObject {
  Q_OBJECT
  QML_SINGLETON
  QML_NAMED_ELEMENT(CompositionEditorVm)
  Q_PROPERTY(bool open READ open NOTIFY openChanged)
  Q_PROPERTY(bool createMode READ createMode NOTIFY editorChanged)
  Q_PROPERTY(QString dialogTitle READ dialogTitle NOTIFY editorChanged)
  Q_PROPERTY(QString saveButtonText READ saveButtonText NOTIFY editorChanged)
  Q_PROPERTY(QString compositionId READ compositionId WRITE setCompositionId NOTIFY editorChanged)
  Q_PROPERTY(QString currentVersion READ currentVersion NOTIFY editorChanged)
  Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY editorChanged)
  Q_PROPERTY(QString revisionComment READ revisionComment WRITE setRevisionComment NOTIFY editorChanged)
  Q_PROPERTY(QString bumpMode READ bumpMode WRITE setBumpMode NOTIFY editorChanged)
  Q_PROPERTY(QStringList bumpOptions READ bumpOptions CONSTANT)
  Q_PROPERTY(QStringList availableBlockIds READ availableBlockIds NOTIFY editorChanged)
  Q_PROPERTY(QString blockSearchText READ blockSearchText WRITE setBlockSearchText NOTIFY editorChanged)
  Q_PROPERTY(QStringList filteredBlockIds READ filteredBlockIds NOTIFY editorChanged)
  Q_PROPERTY(QString blockRefBlockId READ blockRefBlockId WRITE setBlockRefBlockId NOTIFY editorChanged)
  Q_PROPERTY(QString blockRefVersion READ blockRefVersion WRITE setBlockRefVersion NOTIFY editorChanged)
  Q_PROPERTY(QString blockRefParams READ blockRefParams WRITE setBlockRefParams NOTIFY editorChanged)
  Q_PROPERTY(QString staticText READ staticText WRITE setStaticText NOTIFY editorChanged)
  Q_PROPERTY(QString separatorType READ separatorType WRITE setSeparatorType NOTIFY editorChanged)
  Q_PROPERTY(QStringList separatorOptions READ separatorOptions CONSTANT)
  Q_PROPERTY(QAbstractItemModel* fragmentsModel READ fragmentsModel CONSTANT)
  Q_PROPERTY(int fragmentCount READ fragmentCount NOTIFY editorChanged)
  Q_PROPERTY(int selectedFragmentIndex READ selectedFragmentIndex WRITE setSelectedFragmentIndex NOTIFY editorChanged)
  Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY editorChanged)
  Q_PROPERTY(bool hasClipboardFragment READ hasClipboardFragment NOTIFY editorChanged)
  Q_PROPERTY(bool insertModeActive READ insertModeActive NOTIFY editorChanged)
  Q_PROPERTY(QString insertModeLabel READ insertModeLabel NOTIFY editorChanged)
  Q_PROPERTY(QString editorMode READ editorMode WRITE setEditorMode NOTIFY editorChanged)
  Q_PROPERTY(QStringList editorModes READ editorModes CONSTANT)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
  Q_PROPERTY(bool saving READ saving NOTIFY savingChanged)

 public:
  explicit CompositionEditorViewModel(SessionViewModel* session,
                                      CompositionsViewModel* compositions,
                                      QObject* parent = nullptr);
  static CompositionEditorViewModel* create(QQmlEngine* qmlEngine,
                                            QJSEngine* jsEngine);
  static CompositionEditorViewModel* instance();

  bool open() const;
  bool createMode() const;
  QString dialogTitle() const;
  QString saveButtonText() const;
  QString compositionId() const;
  QString currentVersion() const;
  QString description() const;
  QString revisionComment() const;
  QString bumpMode() const;
  QStringList bumpOptions() const;
  QStringList availableBlockIds() const;
  QString blockSearchText() const;
  QStringList filteredBlockIds() const;
  QString blockRefBlockId() const;
  QString blockRefVersion() const;
  QString blockRefParams() const;
  QString staticText() const;
  QString separatorType() const;
  QStringList separatorOptions() const;
  QAbstractItemModel* fragmentsModel();
  int fragmentCount() const;
  int selectedFragmentIndex() const;
  bool hasSelection() const;
  bool hasClipboardFragment() const;
  bool insertModeActive() const;
  QString insertModeLabel() const;
  QString editorMode() const;
  QStringList editorModes() const;
  QString statusText() const;
  bool saving() const;

  void setCompositionId(const QString& value);
  void setDescription(const QString& value);
  void setRevisionComment(const QString& value);
  void setBumpMode(const QString& value);
  void setBlockRefBlockId(const QString& value);
  void setBlockSearchText(const QString& value);
  void setBlockRefVersion(const QString& value);
  void setBlockRefParams(const QString& value);
  void setStaticText(const QString& value);
  void setSeparatorType(const QString& value);
  void setSelectedFragmentIndex(int value);
  void setEditorMode(const QString& value);

  Q_INVOKABLE void openCreateEditor();
  Q_INVOKABLE void openEditor();
  Q_INVOKABLE void closeEditor();
  Q_INVOKABLE void addBlockRef();
  Q_INVOKABLE void addStaticText();
  Q_INVOKABLE void addSeparator();
  Q_INVOKABLE void beginInsertBefore();
  Q_INVOKABLE void beginInsertAfter();
  Q_INVOKABLE void cancelInsertMode();
  Q_INVOKABLE void applySelected();
  Q_INVOKABLE void cutSelected();
  Q_INVOKABLE void pasteBeforeSelected();
  Q_INVOKABLE void pasteAfterSelected();
  Q_INVOKABLE void insertNewlinesBetween();
  Q_INVOKABLE void removeAllSeparators();
  Q_INVOKABLE void removeSelected();
  Q_INVOKABLE void moveFragment(int from, int to);
  Q_INVOKABLE void save();

 signals:
  void openChanged();
  void editorChanged();
  void statusTextChanged();
  void savingChanged();
  void saved();

 private:
  using FragmentSpec = CompositionFragmentsModel::FragmentSpec;

  void resetForm();
  void refreshFilteredBlockIds();
  bool loadSelectedComposition();
  void loadAvailableBlocks();
  void loadEditorFieldsFromSelection();
  void clearFragmentEditorFields();
  void setStatusText(QString value);
  int insertIndexForNewFragment() const;

  [[nodiscard]] std::optional<FragmentSpec> buildFragmentFromEditor(
      FragmentSpec::Kind kind, QString* error_message) const;

  enum class InsertMode { None, Before, After };

  SessionViewModel* session_;
  CompositionsViewModel* compositions_;
  CompositionFragmentsModel fragments_model_;
  bool open_ = false;
  bool create_mode_ = false;
  bool saving_ = false;
  QString composition_id_;
  QString current_version_;
  QString description_;
  QString revision_comment_;
  QString bump_mode_ = QStringLiteral("Minor");
  QStringList available_block_ids_;
  QStringList filtered_block_ids_;
  QString block_search_text_;
  QString block_ref_block_id_;
  QString block_ref_version_;
  QString block_ref_params_;
  QString static_text_;
  QString separator_type_ = QStringLiteral("newline");
  int selected_fragment_index_ = -1;
  InsertMode insert_mode_ = InsertMode::None;
  QString editor_mode_ = QStringLiteral("Block");
  QString status_text_;
  std::optional<FragmentSpec> clipboard_fragment_;
};

}  // namespace tf::gui
