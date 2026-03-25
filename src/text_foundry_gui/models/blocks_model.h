#pragma once

#include <QAbstractItemModel>
#include <QString>
#include <QStringList>
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
  Q_PROPERTY(QString detailsText READ detailsText NOTIFY detailsTextChanged)
  Q_PROPERTY(QString selectedBlockVersion READ selectedBlockVersion NOTIFY detailsTextChanged)
  Q_PROPERTY(QString selectedBlockType READ selectedBlockType NOTIFY detailsTextChanged)
  Q_PROPERTY(QString selectedBlockLanguage READ selectedBlockLanguage NOTIFY detailsTextChanged)
  Q_PROPERTY(QString selectedBlockDescription READ selectedBlockDescription NOTIFY detailsTextChanged)
  Q_PROPERTY(QString selectedBlockTemplate READ selectedBlockTemplate NOTIFY detailsTextChanged)
  Q_PROPERTY(QStringList selectedBlockTags READ selectedBlockTags NOTIFY detailsTextChanged)
  Q_PROPERTY(QStringList selectedBlockDefaults READ selectedBlockDefaults NOTIFY detailsTextChanged)

 public:
  enum Roles {
    NodeTypeRole = Qt::UserRole + 1,
    BlockIdRole,
    IsFolderRole,
    FullPathRole,
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
  QString selectedBlockVersion() const;
  QString selectedBlockType() const;
  QString selectedBlockLanguage() const;
  QString selectedBlockDescription() const;
  QString selectedBlockTemplate() const;
  QStringList selectedBlockTags() const;
  QStringList selectedBlockDefaults() const;

  void setSelectedBlockId(const QString& value);

  Q_INVOKABLE void reload();
  Q_INVOKABLE void selectBlock(const QString& block_id);

 signals:
  void selectedBlockIdChanged();
  void detailsTextChanged();

 private:
  struct Node {
    QString label;
    QString full_path;
    QString block_id;
    bool is_folder = true;
    Node* parent = nullptr;
    std::vector<std::unique_ptr<Node>> children;

    Node* childAt(const int row) const;
    int rowInParent() const;
  };

  void refreshTree();
  void refreshDetails();
  Node* ensureFolderPath(const QStringList& parts);
  Node* firstBlockNode(Node* node) const;
  Node* findBlockNode(const QString& block_id, Node* node) const;
  void sortTree(Node* node);
  const Node* nodeFromIndex(const QModelIndex& index) const;
  Node* nodeFromIndex(const QModelIndex& index);

  SessionViewModel* session_;
  std::unique_ptr<Node> root_;
  QString selected_block_id_;
  QString details_text_ = "Select a block to see details";
  QString selected_block_version_;
  QString selected_block_type_;
  QString selected_block_language_;
  QString selected_block_description_;
  QString selected_block_template_;
  QStringList selected_block_tags_;
  QStringList selected_block_defaults_;
};

}  // namespace tf::gui
