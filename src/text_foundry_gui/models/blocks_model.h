#pragma once

#include <QAbstractItemModel>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqml.h>

#include <memory>
#include <vector>

namespace tf::gui {

class SessionViewModel;

class BlocksModel : public QAbstractItemModel {
  Q_OBJECT
  QML_SINGLETON
  QML_NAMED_ELEMENT(BlocksModel)
  Q_PROPERTY(QString selectedBlockId READ selectedBlockId WRITE setSelectedBlockId NOTIFY selectedBlockIdChanged)
  Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
  Q_PROPERTY(bool showDerivedBlocks READ showDerivedBlocks WRITE setShowDerivedBlocks NOTIFY showDerivedBlocksChanged)
  Q_PROPERTY(QString detailsText READ detailsText NOTIFY detailsTextChanged)
  Q_PROPERTY(QString selectedTreePath READ selectedTreePath NOTIFY treeSelectionChanged)
  Q_PROPERTY(bool selectedTreeIsFolder READ selectedTreeIsFolder NOTIFY treeSelectionChanged)
  Q_PROPERTY(QString selectedBlockVersion READ selectedBlockVersion NOTIFY detailsTextChanged)
  Q_PROPERTY(QStringList selectedBlockVersions READ selectedBlockVersions NOTIFY detailsTextChanged)
  Q_PROPERTY(QStringList selectedBlockVersionOptions READ selectedBlockVersionOptions NOTIFY detailsTextChanged)
  Q_PROPERTY(QVariantList versionEntries READ versionEntries NOTIFY detailsTextChanged)
  Q_PROPERTY(QString selectedBlockType READ selectedBlockType NOTIFY detailsTextChanged)
  Q_PROPERTY(QString selectedBlockLanguage READ selectedBlockLanguage NOTIFY detailsTextChanged)
  Q_PROPERTY(QString selectedBlockDescription READ selectedBlockDescription NOTIFY detailsTextChanged)
  Q_PROPERTY(QString selectedBlockRevisionComment READ selectedBlockRevisionComment NOTIFY detailsTextChanged)
  Q_PROPERTY(QString selectedBlockTemplate READ selectedBlockTemplate NOTIFY detailsTextChanged)
  Q_PROPERTY(QStringList selectedBlockTags READ selectedBlockTags NOTIFY detailsTextChanged)
  Q_PROPERTY(QStringList selectedBlockDefaults READ selectedBlockDefaults NOTIFY detailsTextChanged)

 public:
  enum Roles {
    NodeTypeRole = Qt::UserRole + 1,
    BlockIdRole,
    IsFolderRole,
    FullPathRole,
    HighlightedDisplayRole,
    MatchSnippetRole,
    HighlightedMatchSnippetRole,
  };

  explicit BlocksModel(SessionViewModel* session, QObject* parent = nullptr);
  ~BlocksModel() override;
  static BlocksModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
  static BlocksModel* instance();

  QModelIndex index(int row, int column,
                    const QModelIndex& parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex& child) const override;
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;
  bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;

  QString selectedBlockId() const;
  QString detailsText() const;
  QString selectedTreePath() const;
  bool selectedTreeIsFolder() const;
  QString selectedBlockVersion() const;
  QStringList selectedBlockVersions() const;
  QStringList selectedBlockVersionOptions() const;
  QVariantList versionEntries() const;
  QString selectedBlockType() const;
  QString selectedBlockLanguage() const;
  QString selectedBlockDescription() const;
  QString selectedBlockRevisionComment() const;
  QString selectedBlockTemplate() const;
  QStringList selectedBlockTags() const;
  QStringList selectedBlockDefaults() const;
  QString searchText() const;
  bool showDerivedBlocks() const;

  void setSelectedBlockId(const QString& value);
  void setSearchText(const QString& value);
  void setShowDerivedBlocks(bool value);

  Q_INVOKABLE void reload();
  Q_INVOKABLE void selectBlock(const QString& block_id);
  Q_INVOKABLE void selectTreeItem(const QString& path, bool is_folder,
                                  const QString& block_id = QString());
  Q_INVOKABLE void selectBlockVersion(const QString& version_text);
  Q_INVOKABLE void selectLatestVersion();
  Q_INVOKABLE void deprecateSelected();
  Q_INVOKABLE void deleteSelected();
  Q_INVOKABLE QString highlightSearchText(const QString& text) const;
  Q_INVOKABLE QString highlightSearchContent(const QString& text) const;

 signals:
  void selectedBlockIdChanged();
  void searchTextChanged();
  void showDerivedBlocksChanged();
  void detailsTextChanged();
  void treeReloaded();
  void treeSelectionChanged();

 private:
  struct Node {
    QString label;
    QString full_path;
    QString block_id;
    QString match_snippet;
    bool is_folder = true;
    Node* parent = nullptr;
    std::vector<std::unique_ptr<Node>> children;

    Node* childAt(const int row) const;
    int rowInParent() const;
  };

  void refreshTree();
  void refreshDetails();
  bool removeBlockNode(const QString& block_id);
  void pruneEmptyFolders(Node* start_parent);
  Node* ensureFolderPath(const QStringList& parts);
  Node* firstBlockNode(Node* node) const;
  Node* findBlockNode(const QString& block_id, Node* node) const;
  void sortTree(Node* node);
  QModelIndex indexForNode(const Node* node) const;
  const Node* nodeFromIndex(const QModelIndex& index) const;
  Node* nodeFromIndex(const QModelIndex& index);

  SessionViewModel* session_;
  std::unique_ptr<Node> root_;
  QString selected_tree_path_;
  bool selected_tree_is_folder_ = false;
  QString selected_block_id_;
  QString details_text_ = "Select a block to see details";
  QString selected_block_version_;
  QStringList selected_block_versions_;
  QStringList selected_block_version_options_;
  QVariantList version_entries_;
  QString selected_block_type_;
  QString selected_block_language_;
  QString selected_block_description_;
  QString selected_block_revision_comment_;
  QString selected_block_template_;
  QStringList selected_block_tags_;
  QStringList selected_block_defaults_;
  QString search_text_;
  bool show_derived_blocks_ = false;
};

}  // namespace tf::gui
