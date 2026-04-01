#include "models/blocks_model.h"

#include <QCoreApplication>
#include <algorithm>
#include <array>
#include <sstream>

#include "app/session_view_model.h"

namespace tf::gui {
namespace {

bool IsDerivedBlockId(const QString& block_id) {
  return block_id.contains(QStringLiteral(".norm."));
}

QString HtmlEscaped(const QString& text) { return text.toHtmlEscaped(); }

QString HtmlEscapedPreservingWhitespace(const QString& text) {
  QString escaped = HtmlEscaped(text);
  escaped.replace(QStringLiteral("\t"), QStringLiteral("&nbsp;&nbsp;&nbsp;&nbsp;"));
  escaped.replace(QStringLiteral(" "), QStringLiteral("&nbsp;"));
  escaped.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
  return escaped;
}

QString HighlightMatch(const QString& text, const QString& needle,
                       const bool preserve_whitespace = false) {
  const auto escape = [preserve_whitespace](const QString& value) {
    return preserve_whitespace ? HtmlEscapedPreservingWhitespace(value)
                               : HtmlEscaped(value);
  };

  const QString escaped_text = escape(text);
  if (needle.isEmpty()) {
    return escaped_text;
  }

  const QString lower_text = text.toLower();
  const QString lower_needle = needle.toLower();
  QString result;
  int position = 0;

  while (position < text.size()) {
    const int match = lower_text.indexOf(lower_needle, position);
    if (match < 0) {
      result += escape(text.mid(position));
      break;
    }

    result += escape(text.mid(position, match - position));
    result += QStringLiteral(
                  "<span style=\"background-color:#f6e27a;color:#161616;\">") +
              escape(text.mid(match, needle.size())) +
              QStringLiteral("</span>");
    position = match + needle.size();
  }

  return result;
}

QString SnippetAroundMatch(const QString& text, const QString& needle,
                           const int radius = 36) {
  if (needle.isEmpty() || text.isEmpty()) return {};

  const int match = text.toLower().indexOf(needle.toLower());
  if (match < 0) return {};

  const int start = std::max(0, match - radius);
  const int end = std::min(text.size(), match + needle.size() + radius);
  QString snippet = text.mid(start, end - start).simplified();
  if (start > 0) {
    snippet.prepend(QStringLiteral("..."));
  }
  if (end < text.size()) {
    snippet.append(QStringLiteral("..."));
  }
  return snippet;
}

QString BlockMatchSnippet(const tf::Block& block, const QString& block_id,
                          const QString& needle) {
  const std::array<QString, 4> fields = {
      block_id,
      QString::fromStdString(block.description()),
      QString::fromStdString(block.templ().Content()),
      QString::fromStdString(block.language()),
  };

  for (const auto& field : fields) {
    const QString snippet = SnippetAroundMatch(field, needle);
    if (!snippet.isEmpty()) {
      return snippet;
    }
  }

  for (const auto& tag : block.tags()) {
    const QString snippet = SnippetAroundMatch(QString::fromStdString(tag), needle);
    if (!snippet.isEmpty()) {
      return snippet;
    }
  }
  for (const auto& [key, value] : block.defaults()) {
    const QString combined =
        QString::fromStdString(key) + QStringLiteral("=") +
        QString::fromStdString(value);
    const QString snippet = SnippetAroundMatch(combined, needle);
    if (!snippet.isEmpty()) {
      return snippet;
    }
  }

  return {};
}

bool BlockMatchesSearch(const tf::Block& block, const QString& block_id,
                        const QString& needle, QString* snippet = nullptr) {
  if (needle.isEmpty()) {
    if (snippet != nullptr) {
      snippet->clear();
    }
    return true;
  }

  if (block_id.contains(needle, Qt::CaseInsensitive)) {
    if (snippet != nullptr) {
      *snippet = BlockMatchSnippet(block, block_id, needle);
    }
    return true;
  }
  if (QString::fromStdString(block.description())
          .contains(needle, Qt::CaseInsensitive)) {
    if (snippet != nullptr) {
      *snippet = BlockMatchSnippet(block, block_id, needle);
    }
    return true;
  }
  if (QString::fromStdString(block.templ().Content())
          .contains(needle, Qt::CaseInsensitive)) {
    if (snippet != nullptr) {
      *snippet = BlockMatchSnippet(block, block_id, needle);
    }
    return true;
  }
  if (QString::fromStdString(block.language())
          .contains(needle, Qt::CaseInsensitive)) {
    if (snippet != nullptr) {
      *snippet = BlockMatchSnippet(block, block_id, needle);
    }
    return true;
  }

  for (const auto& tag : block.tags()) {
    if (QString::fromStdString(tag).contains(needle, Qt::CaseInsensitive)) {
      if (snippet != nullptr) {
        *snippet = BlockMatchSnippet(block, block_id, needle);
      }
      return true;
    }
  }
  for (const auto& [key, value] : block.defaults()) {
    if (QString::fromStdString(key).contains(needle, Qt::CaseInsensitive) ||
        QString::fromStdString(value).contains(needle, Qt::CaseInsensitive)) {
      if (snippet != nullptr) {
        *snippet = BlockMatchSnippet(block, block_id, needle);
      }
      return true;
    }
  }

  return false;
}

std::optional<std::string> ParseVersionText(const QString& input, Version& out) {
  const QString trimmed = input.trimmed();
  const auto parts = trimmed.split('.');
  if (parts.size() != 2) {
    return "Version must be major.minor";
  }

  bool major_ok = false;
  bool minor_ok = false;
  const int major = parts[0].toInt(&major_ok);
  const int minor = parts[1].toInt(&minor_ok);
  if (!major_ok || !minor_ok || major < 0 || major > 65535 || minor < 0 ||
      minor > 65535) {
    return "Version must be major.minor in range 0..65535";
  }

  out = Version{static_cast<uint16_t>(major), static_cast<uint16_t>(minor)};
  return std::nullopt;
}

}  // namespace

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
    case HighlightedDisplayRole:
      return HighlightMatch(node->label, search_text_.trimmed());
    case MatchSnippetRole:
      return node->match_snippet;
    case HighlightedMatchSnippetRole:
      return HighlightMatch(node->match_snippet, search_text_.trimmed());
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
      {HighlightedDisplayRole, "highlightedDisplay"},
      {MatchSnippetRole, "matchSnippet"},
      {HighlightedMatchSnippetRole, "highlightedMatchSnippet"},
  };
}

bool BlocksModel::hasChildren(const QModelIndex& parent) const {
  return rowCount(parent) > 0;
}

QString BlocksModel::selectedBlockId() const { return selected_block_id_; }

QString BlocksModel::detailsText() const { return details_text_; }

QString BlocksModel::selectedBlockVersion() const { return selected_block_version_; }

QStringList BlocksModel::selectedBlockVersions() const {
  return selected_block_versions_;
}

QStringList BlocksModel::selectedBlockVersionOptions() const {
  return selected_block_version_options_;
}

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

QString BlocksModel::searchText() const { return search_text_; }

bool BlocksModel::showDerivedBlocks() const { return show_derived_blocks_; }

void BlocksModel::setSelectedBlockId(const QString& value) {
  if (selected_block_id_ == value) return;
  selected_block_id_ = value;
  selected_block_version_.clear();
  emit selectedBlockIdChanged();
  refreshDetails();
}

void BlocksModel::setSearchText(const QString& value) {
  if (search_text_ == value) return;
  search_text_ = value;
  emit searchTextChanged();
  reload();
}

void BlocksModel::setShowDerivedBlocks(const bool value) {
  if (show_derived_blocks_ == value) return;
  show_derived_blocks_ = value;
  emit showDerivedBlocksChanged();
  reload();
}

void BlocksModel::reload() {
  refreshTree();
  refreshDetails();
}

void BlocksModel::selectBlock(const QString& block_id) { setSelectedBlockId(block_id); }

void BlocksModel::selectBlockVersion(const QString& version_text) {
  if (version_text.isEmpty() || selected_block_version_ == version_text) return;
  selected_block_version_ = version_text;
  emit detailsTextChanged();
  refreshDetails();
}

void BlocksModel::selectLatestVersion() {
  if (selected_block_versions_.isEmpty()) return;
  selectBlockVersion(selected_block_versions_.front());
}

QString BlocksModel::highlightSearchText(const QString& text) const {
  return HighlightMatch(text, search_text_.trimmed());
}

QString BlocksModel::highlightSearchContent(const QString& text) const {
  return HighlightMatch(text, search_text_.trimmed(), true);
}

void BlocksModel::deprecateSelected() {
  if (selected_block_id_.isEmpty()) {
    details_text_ = "Select a block first.";
    emit detailsTextChanged();
    return;
  }

  Version version;
  if (const auto parse_error = ParseVersionText(selected_block_version_, version);
      parse_error.has_value()) {
    details_text_ = QString("Error: %1").arg(QString::fromStdString(*parse_error));
    emit detailsTextChanged();
    return;
  }

  const auto error =
      session_->engine().DeprecateBlock(selected_block_id_.toStdString(), version);
  if (error.is_error()) {
    details_text_ =
        QString("Error: %1").arg(QString::fromStdString(error.message));
    emit detailsTextChanged();
    return;
  }

  refreshTree();
  refreshDetails();
}

void BlocksModel::refreshTree() {
  beginResetModel();
  root_ = std::make_unique<Node>();
  root_->label = "/";
  root_->full_path = "/";
  root_->is_folder = true;

  const QString needle = search_text_.trimmed();
  auto ids = session_->engine().ListBlocks();
  std::sort(ids.begin(), ids.end());

  for (const auto& raw_id : ids) {
    const auto block_id = QString::fromStdString(raw_id);
    if (!show_derived_blocks_ && IsDerivedBlockId(block_id)) {
      continue;
    }

    QString snippet;
    if (!needle.isEmpty()) {
      const auto block_result = session_->engine().LoadBlock(raw_id);
      if (block_result.HasError()) {
        continue;
      }
      if (!BlockMatchesSearch(block_result.value(), block_id, needle, &snippet)) {
        continue;
      }
    }

    const auto parts = block_id.split('.', Qt::SkipEmptyParts);
    Node* folder = root_.get();
    if (parts.size() > 1) {
      folder = ensureFolderPath(parts.first(parts.size() - 1));
    }

    auto block = std::make_unique<Node>();
    block->label = parts.isEmpty() ? block_id : parts.back();
    block->full_path = block_id;
    block->block_id = block_id;
    block->match_snippet = snippet;
    block->is_folder = false;
    block->parent = folder;
    folder->children.push_back(std::move(block));
  }

  sortTree(root_.get());

  endResetModel();
  emit treeReloaded();

  if (selected_block_id_.isEmpty()) {
    if (auto* first = firstBlockNode(root_.get())) {
      selected_block_id_ = first->block_id;
      emit selectedBlockIdChanged();
    }
  } else if ((!show_derived_blocks_ && IsDerivedBlockId(selected_block_id_)) ||
             findBlockNode(selected_block_id_, root_.get()) == nullptr) {
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
    selected_block_versions_.clear();
    selected_block_version_options_.clear();
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

  const auto versions_result =
      session_->engine().ListBlockVersions(selected_block_id_.toStdString());
  if (versions_result.HasError()) {
    const auto text = QString("Error: %1")
                          .arg(QString::fromStdString(versions_result.error().message));
    selected_block_version_.clear();
    selected_block_versions_.clear();
    selected_block_version_options_.clear();
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

  selected_block_versions_.clear();
  selected_block_version_options_.clear();
  const auto& versions = versions_result.value();
  for (int i = 0; i < static_cast<int>(versions.size()); ++i) {
    const auto& version = versions[static_cast<size_t>(i)];
    const QString version_text = QString::fromStdString(version.ToString());
    selected_block_versions_.push_back(version_text);

    auto block_result =
        session_->engine().LoadBlock(selected_block_id_.toStdString(), version);
    QString label = version_text;
    QStringList markers;
    if (i == 0) {
      markers.push_back(QStringLiteral("latest"));
    }
    if (!block_result.HasError() &&
        block_result.value().state() == BlockState::Deprecated) {
      markers.push_back(QStringLiteral("deprecated"));
    }
    if (!markers.isEmpty()) {
      label += QStringLiteral(" (%1)").arg(markers.join(QStringLiteral(", ")));
    }
    selected_block_version_options_.push_back(label);
  }
  if (selected_block_version_.isEmpty() ||
      !selected_block_versions_.contains(selected_block_version_)) {
    selected_block_version_ = selected_block_versions_.isEmpty()
                                  ? QString()
                                  : selected_block_versions_.front();
  }

  Version selected_version;
  if (const auto parse_error =
          ParseVersionText(selected_block_version_, selected_version);
      parse_error.has_value()) {
    const auto text = QString("Error: %1").arg(QString::fromStdString(*parse_error));
    if (details_text_ != text) {
      details_text_ = text;
      emit detailsTextChanged();
    }
    return;
  }

  const auto result = session_->engine().LoadBlock(selected_block_id_.toStdString(),
                                                   selected_version);
  if (result.HasError()) {
    const auto text = QString("Error: %1")
                          .arg(QString::fromStdString(result.error().message));
    selected_block_version_.clear();
    selected_block_versions_.clear();
    selected_block_version_options_.clear();
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
