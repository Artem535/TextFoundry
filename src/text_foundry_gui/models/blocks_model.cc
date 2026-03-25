#include "models/blocks_model.h"

#include <QCoreApplication>
#include <algorithm>
#include <sstream>

#include "app/session_view_model.h"

namespace tf::gui {

BlocksModel::BlocksModel(SessionViewModel* session, QObject* parent)
    : QAbstractItemModel(parent),
      session_(session),
      root_(std::make_unique<Node>()) {
  Q_ASSERT(session_ != nullptr);
  connect(session_, &SessionViewModel::engineReset, this, &BlocksModel::reload);
  reload();
}

BlocksModel::~BlocksModel() = default;

BlocksModel* BlocksModel::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine) {
  Q_UNUSED(qmlEngine);
  Q_UNUSED(jsEngine);
  return instance();
}

BlocksModel* BlocksModel::instance() {
  static auto* singleton =
      new BlocksModel(SessionViewModel::instance(), QCoreApplication::instance());
  return singleton;
}

BlocksModel::Node* BlocksModel::Node::childAt(const int row) const {
  if (row < 0 || row >= static_cast<int>(children.size())) return nullptr;
  return children[static_cast<size_t>(row)].get();
}

int BlocksModel::Node::rowInParent() const {
  if (parent == nullptr) return 0;
  for (size_t i = 0; i < parent->children.size(); ++i) {
    if (parent->children[i].get() == this) return static_cast<int>(i);
  }
  return 0;
}

QModelIndex BlocksModel::index(const int row, const int column,
                               const QModelIndex& parent) const {
  if (column != 0 || row < 0) return {};
  const auto* parent_node = nodeFromIndex(parent);
  if (parent_node == nullptr) return {};
  if (auto* child = parent_node->childAt(row)) {
    return createIndex(row, column, child);
  }
  return {};
}

QModelIndex BlocksModel::parent(const QModelIndex& child) const {
  if (!child.isValid()) return {};
  const auto* node = nodeFromIndex(child);
  if (node == nullptr || node->parent == nullptr || node->parent == root_.get()) {
    return {};
  }
  return createIndex(node->parent->rowInParent(), 0, node->parent);
}

int BlocksModel::rowCount(const QModelIndex& parent) const {
  const auto* node = nodeFromIndex(parent);
  if (node == nullptr) return 0;
  return static_cast<int>(node->children.size());
}

int BlocksModel::columnCount(const QModelIndex& parent) const {
  Q_UNUSED(parent);
  return 1;
}

QVariant BlocksModel::data(const QModelIndex& index, const int role) const {
  const auto* node = nodeFromIndex(index);
  if (node == nullptr) return {};

  switch (role) {
    case Qt::DisplayRole:
      return node->label;
    case NodeTypeRole:
      return node->is_folder ? QStringLiteral("folder")
                             : QStringLiteral("block");
    case BlockIdRole:
      return node->block_id;
    case IsFolderRole:
      return node->is_folder;
    case FullPathRole:
      return node->full_path;
    default:
      return {};
  }
}

QHash<int, QByteArray> BlocksModel::roleNames() const {
  return {
      {Qt::DisplayRole, "display"},
      {NodeTypeRole, "nodeType"},
      {BlockIdRole, "blockId"},
      {IsFolderRole, "isFolder"},
      {FullPathRole, "fullPath"},
  };
}

bool BlocksModel::hasChildren(const QModelIndex& parent) const {
  return rowCount(parent) > 0;
}

QString BlocksModel::selectedBlockId() const { return selected_block_id_; }

QString BlocksModel::detailsText() const { return details_text_; }

QString BlocksModel::selectedBlockVersion() const { return selected_block_version_; }

QString BlocksModel::selectedBlockType() const { return selected_block_type_; }

QString BlocksModel::selectedBlockLanguage() const { return selected_block_language_; }

QString BlocksModel::selectedBlockDescription() const {
  return selected_block_description_;
}

QString BlocksModel::selectedBlockTemplate() const { return selected_block_template_; }

QStringList BlocksModel::selectedBlockTags() const { return selected_block_tags_; }

QStringList BlocksModel::selectedBlockDefaults() const {
  return selected_block_defaults_;
}

void BlocksModel::setSelectedBlockId(const QString& value) {
  if (selected_block_id_ == value) return;
  selected_block_id_ = value;
  emit selectedBlockIdChanged();
  refreshDetails();
}

void BlocksModel::reload() {
  refreshTree();
  refreshDetails();
}

void BlocksModel::selectBlock(const QString& block_id) { setSelectedBlockId(block_id); }

void BlocksModel::refreshTree() {
  beginResetModel();
  root_ = std::make_unique<Node>();
  root_->label = "/";
  root_->full_path = "/";
  root_->is_folder = true;

  auto ids = session_->engine().ListBlocks();
  std::sort(ids.begin(), ids.end());

  for (const auto& raw_id : ids) {
    const auto block_id = QString::fromStdString(raw_id);
    const auto parts = block_id.split('.', Qt::SkipEmptyParts);
    Node* folder = root_.get();
    if (parts.size() > 1) {
      folder = ensureFolderPath(parts.first(parts.size() - 1));
    }

    auto block = std::make_unique<Node>();
    block->label = parts.isEmpty() ? block_id : parts.back();
    block->full_path = block_id;
    block->block_id = block_id;
    block->is_folder = false;
    block->parent = folder;
    folder->children.push_back(std::move(block));
  }

  sortTree(root_.get());

  endResetModel();

  if (selected_block_id_.isEmpty()) {
    if (auto* first = firstBlockNode(root_.get())) {
      selected_block_id_ = first->block_id;
      emit selectedBlockIdChanged();
    }
  } else if (findBlockNode(selected_block_id_, root_.get()) == nullptr) {
    if (auto* first = firstBlockNode(root_.get())) {
      selected_block_id_ = first->block_id;
    } else {
      selected_block_id_.clear();
    }
    emit selectedBlockIdChanged();
  }
}

void BlocksModel::refreshDetails() {
  if (selected_block_id_.isEmpty()) {
    const QString text = "No blocks found.";
    selected_block_version_.clear();
    selected_block_type_.clear();
    selected_block_language_.clear();
    selected_block_description_.clear();
    selected_block_template_.clear();
    selected_block_tags_.clear();
    selected_block_defaults_.clear();
    if (details_text_ != text) {
      details_text_ = text;
      emit detailsTextChanged();
    }
    return;
  }

  const auto result = session_->engine().LoadBlock(selected_block_id_.toStdString());
  if (result.HasError()) {
    const auto text = QString("Error: %1")
                          .arg(QString::fromStdString(result.error().message));
    selected_block_version_.clear();
    selected_block_type_.clear();
    selected_block_language_.clear();
    selected_block_description_.clear();
    selected_block_template_.clear();
    selected_block_tags_.clear();
    selected_block_defaults_.clear();
    if (details_text_ != text) {
      details_text_ = text;
      emit detailsTextChanged();
    }
    return;
  }

  const auto& block = result.value();
  selected_block_version_ = QString::fromStdString(block.version().ToString());
  selected_block_type_ =
      QString::fromUtf8(tf::BlockTypeToString(block.type()).data(),
                        static_cast<int>(tf::BlockTypeToString(block.type()).size()));
  selected_block_language_ = QString::fromStdString(block.language());
  selected_block_description_ = QString::fromStdString(block.description());
  selected_block_template_ = QString::fromStdString(block.templ().Content());
  selected_block_tags_.clear();
  for (const auto& tag : block.tags()) {
    selected_block_tags_.push_back(QString::fromStdString(tag));
  }
  std::sort(selected_block_tags_.begin(), selected_block_tags_.end());
  selected_block_defaults_.clear();
  for (const auto& [key, value] : block.defaults()) {
    selected_block_defaults_.push_back(
        QString::fromStdString(key + "=" + value));
  }
  std::sort(selected_block_defaults_.begin(), selected_block_defaults_.end());

  std::ostringstream out;
  out << "id: " << block.Id() << "\n";
  out << "version: " << block.version().ToString() << "\n";
  out << "type: " << tf::BlockTypeToString(block.type()) << "\n";
  if (!block.description().empty()) {
    out << "description: " << block.description() << "\n";
  }
  out << "\ntemplate:\n" << block.templ().Content();

  const auto text = QString::fromStdString(out.str());
  if (details_text_ != text) {
    details_text_ = text;
    emit detailsTextChanged();
  } else {
    emit detailsTextChanged();
  }
}

BlocksModel::Node* BlocksModel::ensureFolderPath(const QStringList& parts) {
  Node* current = root_.get();
  QString full_path;
  for (const auto& part : parts) {
    full_path = full_path.isEmpty() ? part : full_path + "." + part;

    auto it = std::find_if(current->children.begin(), current->children.end(),
                           [&](const std::unique_ptr<Node>& child) {
                             return child->is_folder && child->label == part;
                           });
    if (it == current->children.end()) {
      auto folder = std::make_unique<Node>();
      folder->label = part;
      folder->full_path = full_path;
      folder->is_folder = true;
      folder->parent = current;
      current->children.push_back(std::move(folder));
      it = std::prev(current->children.end());
    }
    current = it->get();
  }
  return current;
}

BlocksModel::Node* BlocksModel::firstBlockNode(Node* node) const {
  if (node == nullptr) return nullptr;
  if (!node->is_folder) return node;
  for (const auto& child : node->children) {
    if (auto* result = firstBlockNode(child.get())) return result;
  }
  return nullptr;
}

BlocksModel::Node* BlocksModel::findBlockNode(const QString& block_id,
                                              Node* node) const {
  if (node == nullptr) return nullptr;
  if (!node->is_folder && node->block_id == block_id) return node;
  for (const auto& child : node->children) {
    if (auto* result = findBlockNode(block_id, child.get())) return result;
  }
  return nullptr;
}

void BlocksModel::sortTree(Node* node) {
  if (node == nullptr) return;

  std::sort(node->children.begin(), node->children.end(),
            [](const std::unique_ptr<Node>& lhs,
               const std::unique_ptr<Node>& rhs) {
              if (lhs->is_folder != rhs->is_folder) {
                return lhs->is_folder && !rhs->is_folder;
              }
              return lhs->label.localeAwareCompare(rhs->label) < 0;
            });

  for (const auto& child : node->children) {
    sortTree(child.get());
  }
}

const BlocksModel::Node* BlocksModel::nodeFromIndex(const QModelIndex& index) const {
  if (!index.isValid()) return root_.get();
  return static_cast<const Node*>(index.internalPointer());
}

BlocksModel::Node* BlocksModel::nodeFromIndex(const QModelIndex& index) {
  if (!index.isValid()) return root_.get();
  return static_cast<Node*>(index.internalPointer());
}

}  // namespace tf::gui
