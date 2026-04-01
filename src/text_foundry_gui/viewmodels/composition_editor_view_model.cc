#include "viewmodels/composition_editor_view_model.h"

#include <QCoreApplication>

#include <algorithm>
#include <cctype>
#include <ranges>
#include <sstream>
#include <string>

#include "app/session_view_model.h"
#include "tf/block_ref.h"
#include "tf/composition.h"
#include "tf/fragment.h"
#include "viewmodels/compositions_view_model.h"

namespace tf::gui {
namespace {

using FragmentSpec = CompositionFragmentsModel::FragmentSpec;

std::string Trim(std::string value) {
  const auto is_space = [](const unsigned char c) { return std::isspace(c); };
  while (!value.empty() && is_space(static_cast<unsigned char>(value.front()))) {
    value.erase(value.begin());
  }
  while (!value.empty() && is_space(static_cast<unsigned char>(value.back()))) {
    value.pop_back();
  }
  return value;
}

QString FirstLine(QString text) {
  text.replace('\n', ' ');
  text = text.simplified();
  if (text.size() > 96) {
    text = text.left(93) + "...";
  }
  return text;
}

std::optional<std::string> ParseVersionText(const QString& input, Version& out) {
  const std::string text = Trim(input.toStdString());
  if (text.empty()) {
    return "Version must be major.minor";
  }

  const auto dot = text.find('.');
  if (dot == std::string::npos) {
    return "Version must be major.minor";
  }

  const std::string major_str = Trim(text.substr(0, dot));
  const std::string minor_str = Trim(text.substr(dot + 1));
  if (major_str.empty() || minor_str.empty()) {
    return "Version must be major.minor";
  }

  try {
    const auto major = std::stoi(major_str);
    const auto minor = std::stoi(minor_str);
    if (major < 0 || major > 65535 || minor < 0 || minor > 65535) {
      return "Version values must be in 0..65535";
    }
    out = Version{static_cast<uint16_t>(major), static_cast<uint16_t>(minor)};
  } catch (const std::exception&) {
    return "Version must be numeric";
  }

  return std::nullopt;
}

Result<Params> ParseParamsText(const QString& input) {
  Params out;
  std::string raw = input.toStdString();
  std::ranges::replace(raw, '&', '\n');
  std::ranges::replace(raw, ',', '\n');
  std::istringstream stream(raw);
  std::string line;
  while (std::getline(stream, line)) {
    line = Trim(line);
    if (line.empty()) continue;
    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      return Result<Params>(
          Error{ErrorCode::InvalidParamType,
                "Params must use key=value format"});
    }
    const std::string key = Trim(line.substr(0, eq));
    const std::string value = Trim(line.substr(eq + 1));
    if (key.empty()) {
      return Result<Params>(
          Error{ErrorCode::InvalidParamType, "Param name cannot be empty"});
    }
    out[key] = value;
  }
  return Result<Params>(out);
}

Engine::VersionBump ParseBumpMode(const QString& value) {
  return value == QStringLiteral("Major") ? Engine::VersionBump::Major
                                          : Engine::VersionBump::Minor;
}

SeparatorType ParseSeparatorType(const QString& value) {
  if (value == QStringLiteral("paragraph")) return SeparatorType::Paragraph;
  if (value == QStringLiteral("hr")) return SeparatorType::Hr;
  return SeparatorType::Newline;
}

FragmentSpec::Kind ParseEditorMode(const QString& value) {
  if (value == QStringLiteral("Text")) return FragmentSpec::Kind::StaticText;
  if (value == QStringLiteral("Separator")) return FragmentSpec::Kind::Separator;
  return FragmentSpec::Kind::BlockRef;
}

QString EditorModeForKind(const FragmentSpec::Kind kind) {
  switch (kind) {
    case FragmentSpec::Kind::BlockRef:
      return QStringLiteral("Block");
    case FragmentSpec::Kind::StaticText:
      return QStringLiteral("Text");
    case FragmentSpec::Kind::Separator:
      return QStringLiteral("Separator");
  }
  return QStringLiteral("Block");
}

QString KindLabelFor(const FragmentSpec::Kind kind) {
  switch (kind) {
    case FragmentSpec::Kind::BlockRef:
      return QStringLiteral("Block");
    case FragmentSpec::Kind::StaticText:
      return QStringLiteral("Text");
    case FragmentSpec::Kind::Separator:
      return QStringLiteral("Separator");
  }
  return {};
}

}  // namespace

CompositionFragmentsModel::CompositionFragmentsModel(QObject* parent)
    : QAbstractListModel(parent) {}

int CompositionFragmentsModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(fragments_.size());
}

QVariant CompositionFragmentsModel::data(const QModelIndex& index,
                                         const int role) const {
  if (!index.isValid() || index.row() < 0 ||
      index.row() >= static_cast<int>(fragments_.size())) {
    return {};
  }
  return dataForFragment(fragments_[static_cast<size_t>(index.row())], role);
}

QHash<int, QByteArray> CompositionFragmentsModel::roleNames() const {
  return {
      {KindRole, "kind"},
      {KindLabelRole, "kindLabel"},
      {TitleRole, "title"},
      {MetaRole, "meta"},
      {BodyRole, "body"},
  };
}

const std::vector<CompositionFragmentsModel::FragmentSpec>&
CompositionFragmentsModel::fragments() const noexcept {
  return fragments_;
}

const CompositionFragmentsModel::FragmentSpec*
CompositionFragmentsModel::fragmentAt(const int index) const noexcept {
  if (index < 0 || index >= static_cast<int>(fragments_.size())) {
    return nullptr;
  }
  return &fragments_[static_cast<size_t>(index)];
}

void CompositionFragmentsModel::setFragments(std::vector<FragmentSpec> fragments) {
  beginResetModel();
  fragments_ = std::move(fragments);
  endResetModel();
}

void CompositionFragmentsModel::appendFragment(FragmentSpec fragment) {
  const auto row = static_cast<int>(fragments_.size());
  beginInsertRows({}, row, row);
  fragments_.push_back(std::move(fragment));
  endInsertRows();
}

void CompositionFragmentsModel::insertFragment(const int index,
                                               FragmentSpec fragment) {
  const auto row =
      std::clamp(index, 0, static_cast<int>(fragments_.size()));
  beginInsertRows({}, row, row);
  fragments_.insert(fragments_.begin() + row, std::move(fragment));
  endInsertRows();
}

void CompositionFragmentsModel::updateFragment(const int index,
                                               FragmentSpec fragment) {
  if (index < 0 || index >= static_cast<int>(fragments_.size())) {
    return;
  }
  fragments_[static_cast<size_t>(index)] = std::move(fragment);
  emit dataChanged(this->index(index), this->index(index));
}

void CompositionFragmentsModel::removeFragment(const int index) {
  if (index < 0 || index >= static_cast<int>(fragments_.size())) {
    return;
  }
  beginRemoveRows({}, index, index);
  fragments_.erase(fragments_.begin() + index);
  endRemoveRows();
}

void CompositionFragmentsModel::moveFragment(const int from, const int to) {
  if (from < 0 || to < 0 || from >= static_cast<int>(fragments_.size()) ||
      to >= static_cast<int>(fragments_.size()) || from == to) {
    return;
  }

  const auto destination = from < to ? to + 1 : to;
  beginMoveRows({}, from, from, {}, destination);
  auto fragment = std::move(fragments_[static_cast<size_t>(from)]);
  fragments_.erase(fragments_.begin() + from);
  fragments_.insert(fragments_.begin() + to, std::move(fragment));
  endMoveRows();
}

QVariant CompositionFragmentsModel::dataForFragment(const FragmentSpec& fragment,
                                                    const int role) const {
  switch (role) {
    case KindRole:
      return EditorModeForKind(fragment.kind).toLower();
    case KindLabelRole:
      return KindLabelFor(fragment.kind);
    case TitleRole:
      switch (fragment.kind) {
        case FragmentSpec::Kind::BlockRef:
          return fragment.block_id;
        case FragmentSpec::Kind::StaticText:
          return FirstLine(fragment.text);
        case FragmentSpec::Kind::Separator:
          return QString("%1 separator")
              .arg(fragment.separator.isEmpty() ? QStringLiteral("newline")
                                                : fragment.separator);
      }
      break;
    case MetaRole:
      switch (fragment.kind) {
        case FragmentSpec::Kind::BlockRef: {
          QStringList parts;
          parts << QString("v%1").arg(fragment.version);
          if (!fragment.params.trimmed().isEmpty()) {
            parts << QStringLiteral("local params");
          }
          return parts.join(QStringLiteral(" • "));
        }
        case FragmentSpec::Kind::StaticText:
          return QString("%1 chars").arg(fragment.text.size());
        case FragmentSpec::Kind::Separator:
          return QStringLiteral("Structural separator");
      }
      break;
    case BodyRole:
      switch (fragment.kind) {
        case FragmentSpec::Kind::BlockRef:
          return fragment.params.trimmed().isEmpty() ? QStringLiteral("No local params")
                                                     : fragment.params;
        case FragmentSpec::Kind::StaticText:
          return fragment.text;
        case FragmentSpec::Kind::Separator:
          if (fragment.separator == QStringLiteral("paragraph")) {
            return QStringLiteral("Adds paragraph spacing between fragments.");
          }
          if (fragment.separator == QStringLiteral("hr")) {
            return QStringLiteral("Adds a horizontal-rule style separator.");
          }
          return QStringLiteral("Adds a single newline between fragments.");
      }
      break;
    default:
      break;
  }

  return {};
}

CompositionEditorViewModel::CompositionEditorViewModel(
    SessionViewModel* session, CompositionsViewModel* compositions,
    QObject* parent)
    : QObject(parent),
      session_(session),
      compositions_(compositions),
      fragments_model_(this) {
  Q_ASSERT(session_ != nullptr);
  Q_ASSERT(compositions_ != nullptr);
}

CompositionEditorViewModel* CompositionEditorViewModel::create(
    QQmlEngine* qmlEngine, QJSEngine* jsEngine) {
  Q_UNUSED(qmlEngine);
  Q_UNUSED(jsEngine);
  return instance();
}

CompositionEditorViewModel* CompositionEditorViewModel::instance() {
  static auto* singleton = new CompositionEditorViewModel(
      SessionViewModel::instance(), CompositionsViewModel::instance(),
      QCoreApplication::instance());
  return singleton;
}

bool CompositionEditorViewModel::open() const { return open_; }

bool CompositionEditorViewModel::createMode() const { return create_mode_; }

QString CompositionEditorViewModel::dialogTitle() const {
  return create_mode_ ? QStringLiteral("Create Composition")
                      : QStringLiteral("Edit Composition");
}

QString CompositionEditorViewModel::saveButtonText() const {
  return create_mode_ ? QStringLiteral("Create") : QStringLiteral("Save");
}

QString CompositionEditorViewModel::compositionId() const { return composition_id_; }

QString CompositionEditorViewModel::currentVersion() const {
  return current_version_;
}

QString CompositionEditorViewModel::description() const { return description_; }

QString CompositionEditorViewModel::revisionComment() const {
  return revision_comment_;
}

QString CompositionEditorViewModel::bumpMode() const { return bump_mode_; }

QStringList CompositionEditorViewModel::bumpOptions() const {
  return {QStringLiteral("Minor"), QStringLiteral("Major")};
}

QStringList CompositionEditorViewModel::availableBlockIds() const {
  return available_block_ids_;
}

QString CompositionEditorViewModel::blockSearchText() const {
  return block_search_text_;
}

QStringList CompositionEditorViewModel::filteredBlockIds() const {
  return filtered_block_ids_;
}

QString CompositionEditorViewModel::blockRefBlockId() const {
  return block_ref_block_id_;
}

QString CompositionEditorViewModel::blockRefVersion() const {
  return block_ref_version_;
}

QString CompositionEditorViewModel::blockRefParams() const {
  return block_ref_params_;
}

QString CompositionEditorViewModel::staticText() const { return static_text_; }

QString CompositionEditorViewModel::separatorType() const {
  return separator_type_;
}

QStringList CompositionEditorViewModel::separatorOptions() const {
  return {QStringLiteral("newline"), QStringLiteral("paragraph"),
          QStringLiteral("hr")};
}

QAbstractItemModel* CompositionEditorViewModel::fragmentsModel() {
  return &fragments_model_;
}

int CompositionEditorViewModel::fragmentCount() const {
  return fragments_model_.rowCount();
}

int CompositionEditorViewModel::selectedFragmentIndex() const {
  return selected_fragment_index_;
}

bool CompositionEditorViewModel::hasSelection() const {
  return selected_fragment_index_ >= 0 &&
         selected_fragment_index_ < fragments_model_.rowCount();
}

bool CompositionEditorViewModel::hasClipboardFragment() const {
  return clipboard_fragment_.has_value();
}

bool CompositionEditorViewModel::insertModeActive() const {
  return insert_mode_ != InsertMode::None;
}

QString CompositionEditorViewModel::insertModeLabel() const {
  switch (insert_mode_) {
    case InsertMode::Before:
      return QStringLiteral("Create new fragment before selection");
    case InsertMode::After:
      return QStringLiteral("Create new fragment after selection");
    case InsertMode::None:
      break;
  }
  return QStringLiteral("Edit selected fragment");
}

QString CompositionEditorViewModel::editorMode() const { return editor_mode_; }

QStringList CompositionEditorViewModel::editorModes() const {
  return {QStringLiteral("Block"), QStringLiteral("Text"),
          QStringLiteral("Separator")};
}

QString CompositionEditorViewModel::statusText() const { return status_text_; }

bool CompositionEditorViewModel::saving() const { return saving_; }

void CompositionEditorViewModel::setCompositionId(const QString& value) {
  if (composition_id_ == value) return;
  composition_id_ = value;
  emit editorChanged();
}

void CompositionEditorViewModel::setDescription(const QString& value) {
  if (description_ == value) return;
  description_ = value;
  emit editorChanged();
}

void CompositionEditorViewModel::setRevisionComment(const QString& value) {
  if (revision_comment_ == value) return;
  revision_comment_ = value;
  emit editorChanged();
}

void CompositionEditorViewModel::setBumpMode(const QString& value) {
  if (bump_mode_ == value) return;
  bump_mode_ = value;
  emit editorChanged();
}

void CompositionEditorViewModel::setBlockRefBlockId(const QString& value) {
  if (block_ref_block_id_ == value) return;
  block_ref_block_id_ = value;
  emit editorChanged();
}

void CompositionEditorViewModel::setBlockSearchText(const QString& value) {
  if (block_search_text_ == value) return;
  block_search_text_ = value;
  refreshFilteredBlockIds();
  emit editorChanged();
}

void CompositionEditorViewModel::setBlockRefVersion(const QString& value) {
  if (block_ref_version_ == value) return;
  block_ref_version_ = value;
  emit editorChanged();
}

void CompositionEditorViewModel::setBlockRefParams(const QString& value) {
  if (block_ref_params_ == value) return;
  block_ref_params_ = value;
  emit editorChanged();
}

void CompositionEditorViewModel::setStaticText(const QString& value) {
  if (static_text_ == value) return;
  static_text_ = value;
  emit editorChanged();
}

void CompositionEditorViewModel::setSeparatorType(const QString& value) {
  if (separator_type_ == value) return;
  separator_type_ = value;
  emit editorChanged();
}

void CompositionEditorViewModel::setSelectedFragmentIndex(const int value) {
  if (selected_fragment_index_ == value) return;
  selected_fragment_index_ = value;
  loadEditorFieldsFromSelection();
  emit editorChanged();
}

void CompositionEditorViewModel::setEditorMode(const QString& value) {
  if (editor_mode_ == value) return;
  editor_mode_ = value;
  emit editorChanged();
}

void CompositionEditorViewModel::openCreateEditor() {
  resetForm();
  loadAvailableBlocks();
  create_mode_ = true;
  open_ = true;
  setStatusText(QStringLiteral("Assemble the composition and press Create."));
  emit editorChanged();
  emit openChanged();
}

void CompositionEditorViewModel::openEditor() {
  if (!loadSelectedComposition()) return;
  create_mode_ = false;
  open_ = true;
  emit editorChanged();
  emit openChanged();
}

void CompositionEditorViewModel::closeEditor() {
  if (!open_) return;
  open_ = false;
  emit openChanged();
}

void CompositionEditorViewModel::addBlockRef() {
  QString error_message;
  auto fragment =
      buildFragmentFromEditor(FragmentSpec::Kind::BlockRef, &error_message);
  if (!fragment.has_value()) {
    setStatusText(error_message);
    return;
  }

  fragments_model_.appendFragment(std::move(*fragment));
  selected_fragment_index_ = fragments_model_.rowCount() - 1;
  insert_mode_ = InsertMode::None;
  setStatusText(QStringLiteral("Added block fragment."));
  emit editorChanged();
}

void CompositionEditorViewModel::addStaticText() {
  QString error_message;
  auto fragment =
      buildFragmentFromEditor(FragmentSpec::Kind::StaticText, &error_message);
  if (!fragment.has_value()) {
    setStatusText(error_message);
    return;
  }

  fragments_model_.appendFragment(std::move(*fragment));
  selected_fragment_index_ = fragments_model_.rowCount() - 1;
  insert_mode_ = InsertMode::None;
  setStatusText(QStringLiteral("Added text fragment."));
  emit editorChanged();
}

void CompositionEditorViewModel::addSeparator() {
  QString error_message;
  auto fragment =
      buildFragmentFromEditor(FragmentSpec::Kind::Separator, &error_message);
  if (!fragment.has_value()) {
    setStatusText(error_message);
    return;
  }

  fragments_model_.appendFragment(std::move(*fragment));
  selected_fragment_index_ = fragments_model_.rowCount() - 1;
  insert_mode_ = InsertMode::None;
  setStatusText(QStringLiteral("Added separator fragment."));
  emit editorChanged();
}

void CompositionEditorViewModel::beginInsertBefore() {
  if (!hasSelection()) {
    setStatusText(QStringLiteral("Select a fragment first."));
    return;
  }
  insert_mode_ = InsertMode::Before;
  clearFragmentEditorFields();
  editor_mode_ = QStringLiteral("Block");
  setStatusText(QStringLiteral("Create mode: new fragment will be inserted before selection."));
  emit editorChanged();
}

void CompositionEditorViewModel::beginInsertAfter() {
  if (!hasSelection()) {
    setStatusText(QStringLiteral("Select a fragment first."));
    return;
  }
  insert_mode_ = InsertMode::After;
  clearFragmentEditorFields();
  editor_mode_ = QStringLiteral("Block");
  setStatusText(QStringLiteral("Create mode: new fragment will be inserted after selection."));
  emit editorChanged();
}

void CompositionEditorViewModel::cancelInsertMode() {
  if (insert_mode_ == InsertMode::None) {
    return;
  }
  insert_mode_ = InsertMode::None;
  loadEditorFieldsFromSelection();
  setStatusText(QStringLiteral("Returned to edit mode."));
  emit editorChanged();
}

void CompositionEditorViewModel::applySelected() {
  if (!hasSelection()) {
    setStatusText(QStringLiteral("Select a fragment first."));
    return;
  }

  QString error_message;
  auto fragment = buildFragmentFromEditor(ParseEditorMode(editor_mode_),
                                          &error_message);
  if (!fragment.has_value()) {
    setStatusText(error_message);
    return;
  }

  if (insert_mode_ == InsertMode::Before || insert_mode_ == InsertMode::After) {
    const int target_index = insert_mode_ == InsertMode::Before
                                 ? selected_fragment_index_
                                 : selected_fragment_index_ + 1;
    fragments_model_.insertFragment(target_index, std::move(*fragment));
    selected_fragment_index_ = target_index;
    insert_mode_ = InsertMode::None;
    loadEditorFieldsFromSelection();
    setStatusText(QStringLiteral("Inserted new fragment relative to selection."));
  } else {
    fragments_model_.updateFragment(selected_fragment_index_, std::move(*fragment));
    loadEditorFieldsFromSelection();
    setStatusText(QStringLiteral("Updated selected fragment."));
  }
  emit editorChanged();
}

void CompositionEditorViewModel::cutSelected() {
  if (!hasSelection()) {
    setStatusText(QStringLiteral("Select a fragment first."));
    return;
  }

  const auto* fragment = fragments_model_.fragmentAt(selected_fragment_index_);
  if (fragment == nullptr) {
    setStatusText(QStringLiteral("Select a fragment first."));
    return;
  }

  clipboard_fragment_ = *fragment;
  fragments_model_.removeFragment(selected_fragment_index_);
  if (fragments_model_.rowCount() == 0) {
    selected_fragment_index_ = -1;
    insert_mode_ = InsertMode::None;
    clearFragmentEditorFields();
  } else {
    selected_fragment_index_ =
        std::min(selected_fragment_index_, fragments_model_.rowCount() - 1);
    loadEditorFieldsFromSelection();
  }
  setStatusText(QStringLiteral("Cut selected fragment."));
  emit editorChanged();
}

void CompositionEditorViewModel::pasteBeforeSelected() {
  if (!clipboard_fragment_.has_value()) {
    setStatusText(QStringLiteral("Clipboard is empty."));
    return;
  }

  const auto target_index = hasSelection() ? selected_fragment_index_ : 0;
  fragments_model_.insertFragment(target_index, *clipboard_fragment_);
  selected_fragment_index_ = target_index;
  loadEditorFieldsFromSelection();
  setStatusText(QStringLiteral("Pasted fragment before selection."));
  emit editorChanged();
}

void CompositionEditorViewModel::pasteAfterSelected() {
  if (!clipboard_fragment_.has_value()) {
    setStatusText(QStringLiteral("Clipboard is empty."));
    return;
  }

  const auto target_index =
      hasSelection() ? selected_fragment_index_ + 1 : fragments_model_.rowCount();
  fragments_model_.insertFragment(target_index, *clipboard_fragment_);
  selected_fragment_index_ = target_index;
  loadEditorFieldsFromSelection();
  setStatusText(QStringLiteral("Pasted fragment after selection."));
  emit editorChanged();
}

void CompositionEditorViewModel::insertNewlinesBetween() {
  const auto& fragments = fragments_model_.fragments();
  if (fragments.size() < 2) {
    setStatusText(QStringLiteral("Need at least two fragments."));
    return;
  }

  std::vector<FragmentSpec> updated;
  updated.reserve(fragments.size() * 2);
  for (size_t i = 0; i < fragments.size(); ++i) {
    updated.push_back(fragments[i]);
    if (i + 1 >= fragments.size()) {
      continue;
    }

    const bool left_is_separator =
        fragments[i].kind == FragmentSpec::Kind::Separator;
    const bool right_is_separator =
        fragments[i + 1].kind == FragmentSpec::Kind::Separator;
    if (!left_is_separator && !right_is_separator) {
      updated.push_back(FragmentSpec{
          .kind = FragmentSpec::Kind::Separator,
          .separator = QStringLiteral("newline"),
      });
    }
  }

  fragments_model_.setFragments(std::move(updated));
  if (selected_fragment_index_ >= fragments_model_.rowCount()) {
    selected_fragment_index_ = fragments_model_.rowCount() - 1;
  }
  if (hasSelection()) {
    loadEditorFieldsFromSelection();
  }
  setStatusText(QStringLiteral("Inserted newline separators between fragments."));
  emit editorChanged();
}

void CompositionEditorViewModel::removeAllSeparators() {
  const auto& fragments = fragments_model_.fragments();
  const bool has_separator = std::ranges::any_of(
      fragments, [](const FragmentSpec& fragment) {
        return fragment.kind == FragmentSpec::Kind::Separator;
      });
  if (!has_separator) {
    setStatusText(QStringLiteral("No separators to remove."));
    return;
  }

  std::vector<FragmentSpec> updated;
  updated.reserve(fragments.size());
  for (const auto& fragment : fragments) {
    if (fragment.kind != FragmentSpec::Kind::Separator) {
      updated.push_back(fragment);
    }
  }

  fragments_model_.setFragments(std::move(updated));
  if (fragments_model_.rowCount() == 0) {
    selected_fragment_index_ = -1;
    insert_mode_ = InsertMode::None;
    clearFragmentEditorFields();
  } else if (selected_fragment_index_ >= fragments_model_.rowCount()) {
    selected_fragment_index_ = fragments_model_.rowCount() - 1;
    loadEditorFieldsFromSelection();
  } else if (hasSelection()) {
    loadEditorFieldsFromSelection();
  }
  setStatusText(QStringLiteral("Removed all separators."));
  emit editorChanged();
}

void CompositionEditorViewModel::removeSelected() {
  if (!hasSelection()) {
    setStatusText(QStringLiteral("Select a fragment first."));
    return;
  }

  fragments_model_.removeFragment(selected_fragment_index_);
  if (fragments_model_.rowCount() == 0) {
    selected_fragment_index_ = -1;
    insert_mode_ = InsertMode::None;
    clearFragmentEditorFields();
  } else {
    selected_fragment_index_ =
        std::min(selected_fragment_index_, fragments_model_.rowCount() - 1);
    loadEditorFieldsFromSelection();
  }
  setStatusText(QStringLiteral("Removed selected fragment."));
  emit editorChanged();
}

void CompositionEditorViewModel::moveFragment(const int from, const int to) {
  if (from == to || from < 0 || to < 0 || from >= fragments_model_.rowCount() ||
      to >= fragments_model_.rowCount()) {
    return;
  }

  fragments_model_.moveFragment(from, to);
  if (selected_fragment_index_ == from) {
    selected_fragment_index_ = to;
  } else if (from < selected_fragment_index_ && to >= selected_fragment_index_) {
    --selected_fragment_index_;
  } else if (from > selected_fragment_index_ && to <= selected_fragment_index_) {
    ++selected_fragment_index_;
  }
  emit editorChanged();
}

void CompositionEditorViewModel::save() {
  if (composition_id_.trimmed().isEmpty()) {
    setStatusText(QStringLiteral("Composition id is required."));
    return;
  }

  saving_ = true;
  emit savingChanged();

  CompositionDraftBuilder builder(composition_id_.trimmed().toStdString());
  builder.WithDescription(description_.toStdString())
      .WithRevisionComment(revision_comment_.trimmed().toStdString())
      .WithProjectKey(session_->projectKey().toStdString());

  const bool has_separator_specs = std::ranges::any_of(
      fragments_model_.fragments(), [](const FragmentSpec& fragment) {
        return fragment.kind == FragmentSpec::Kind::Separator;
      });
  if (session_->compositionNewlineDelimiter() && !has_separator_specs) {
    auto style = StyleProfile::plain();
    style.structural.delimiter = "\n";
    builder.WithStyleProfile(std::move(style));
  }

  for (const auto& fragment : fragments_model_.fragments()) {
    if (fragment.kind == FragmentSpec::Kind::BlockRef) {
      Version version;
      if (const auto parse_error = ParseVersionText(fragment.version, version);
          parse_error.has_value()) {
        saving_ = false;
        emit savingChanged();
        setStatusText(QString::fromStdString(*parse_error));
        return;
      }

      auto params = ParseParamsText(fragment.params);
      if (params.HasError()) {
        saving_ = false;
        emit savingChanged();
        setStatusText(QString::fromStdString(params.error().message));
        return;
      }

      builder.AddBlockRef(fragment.block_id.toStdString(), version.major,
                          version.minor, params.value());
    } else if (fragment.kind == FragmentSpec::Kind::StaticText) {
      builder.AddStaticText(fragment.text.toStdString());
    } else {
      builder.AddSeparator(ParseSeparatorType(fragment.separator));
    }
  }

  auto result = create_mode_
                    ? session_->engine().PublishComposition(
                          builder.build(), ParseBumpMode(bump_mode_))
                    : session_->engine().UpdateComposition(
                          builder.build(), ParseBumpMode(bump_mode_));

  saving_ = false;
  emit savingChanged();

  if (result.HasError()) {
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(result.error().message)));
    return;
  }

  compositions_->reload();
  compositions_->selectComposition(composition_id_);
  setStatusText(QString("%1 %2 as version %3")
                    .arg(create_mode_ ? QStringLiteral("Created")
                                      : QStringLiteral("Saved"),
                         composition_id_,
                         QString::fromStdString(result.value().version().ToString())));
  emit saved();
  closeEditor();
}

void CompositionEditorViewModel::resetForm() {
  composition_id_.clear();
  current_version_.clear();
  description_.clear();
  revision_comment_.clear();
  bump_mode_ = QStringLiteral("Minor");
  available_block_ids_.clear();
  filtered_block_ids_.clear();
  block_search_text_.clear();
  clearFragmentEditorFields();
  fragments_model_.setFragments({});
  selected_fragment_index_ = -1;
  insert_mode_ = InsertMode::None;
  editor_mode_ = QStringLiteral("Block");
}

bool CompositionEditorViewModel::loadSelectedComposition() {
  const QString selected_id = compositions_->selectedCompositionId();
  if (selected_id.isEmpty()) {
    setStatusText(QStringLiteral("Select a composition first."));
    return false;
  }

  resetForm();
  loadAvailableBlocks();

  Version version;
  if (const auto parse_error =
          ParseVersionText(compositions_->selectedVersion(), version);
      parse_error.has_value()) {
    setStatusText(QString("Error: %1").arg(QString::fromStdString(*parse_error)));
    return false;
  }

  auto result =
      session_->engine().LoadComposition(selected_id.toStdString(), version);
  if (result.HasError()) {
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(result.error().message)));
    return false;
  }

  const auto& composition = result.value();
  composition_id_ = selected_id;
  current_version_ = QString::fromStdString(composition.version().ToString());
  description_ = QString::fromStdString(composition.description());
  revision_comment_.clear();

  std::vector<FragmentSpec> fragments;
  fragments.reserve(composition.fragments().size());
  for (const auto& fragment : composition.fragments()) {
    if (fragment.IsBlockRef()) {
      const auto& ref = fragment.AsBlockRef();
      fragments.push_back(FragmentSpec{
          .kind = FragmentSpec::Kind::BlockRef,
          .block_id = QString::fromStdString(ref.GetBlockId()),
          .version = ref.version().has_value()
                         ? QString::fromStdString(ref.version()->ToString())
                         : QString(),
          .params = [&]() {
            QStringList params;
            for (const auto& [key, value] : ref.LocalParams()) {
              params.push_back(QString::fromStdString(key + "=" + value));
            }
            return params.join(QStringLiteral(", "));
          }(),
      });
      continue;
    }

    if (fragment.IsStaticText()) {
      fragments.push_back(FragmentSpec{
          .kind = FragmentSpec::Kind::StaticText,
          .text = QString::fromStdString(fragment.AsStaticText().text()),
      });
      continue;
    }

    const auto name = SeparatorTypeToString(fragment.AsSeparator().type);
    fragments.push_back(FragmentSpec{
        .kind = FragmentSpec::Kind::Separator,
        .separator = QString::fromUtf8(name.data(),
                                       static_cast<int>(name.size())),
    });
  }

  fragments_model_.setFragments(std::move(fragments));
  setStatusText(QStringLiteral("Editing selected version. Saving will publish a new latest version."));
  return true;
}

void CompositionEditorViewModel::loadAvailableBlocks() {
  available_block_ids_.clear();
  for (const auto& id : session_->engine().ListBlocks()) {
    available_block_ids_.push_back(QString::fromStdString(id));
  }
  std::sort(available_block_ids_.begin(), available_block_ids_.end());
  refreshFilteredBlockIds();
  if (block_ref_block_id_.isEmpty() && !available_block_ids_.isEmpty()) {
    block_ref_block_id_ = available_block_ids_.front();
  }
}

void CompositionEditorViewModel::refreshFilteredBlockIds() {
  if (block_search_text_.trimmed().isEmpty()) {
    filtered_block_ids_ = available_block_ids_;
    return;
  }

  const QString needle = block_search_text_.trimmed();
  QStringList filtered;
  filtered.reserve(available_block_ids_.size());
  for (const auto& block_id : available_block_ids_) {
    if (block_id.contains(needle, Qt::CaseInsensitive)) {
      filtered.push_back(block_id);
    }
  }
  filtered_block_ids_ = std::move(filtered);
}

void CompositionEditorViewModel::loadEditorFieldsFromSelection() {
  const auto* fragment = fragments_model_.fragmentAt(selected_fragment_index_);
  if (fragment == nullptr) {
    return;
  }

  editor_mode_ = EditorModeForKind(fragment->kind);
  switch (fragment->kind) {
    case FragmentSpec::Kind::BlockRef:
      block_ref_block_id_ = fragment->block_id;
      block_ref_version_ = fragment->version;
      block_ref_params_ = fragment->params;
      break;
    case FragmentSpec::Kind::StaticText:
      static_text_ = fragment->text;
      break;
    case FragmentSpec::Kind::Separator:
      separator_type_ = fragment->separator;
      break;
  }
}

void CompositionEditorViewModel::clearFragmentEditorFields() {
  block_ref_block_id_ = available_block_ids_.isEmpty() ? QString()
                                                       : available_block_ids_.front();
  block_ref_version_.clear();
  block_ref_params_.clear();
  static_text_.clear();
  separator_type_ = QStringLiteral("newline");
}

void CompositionEditorViewModel::setStatusText(QString value) {
  if (status_text_ == value) return;
  status_text_ = std::move(value);
  emit statusTextChanged();
}

int CompositionEditorViewModel::insertIndexForNewFragment() const {
  if (!hasSelection()) {
    return fragments_model_.rowCount();
  }
  return insert_mode_ == InsertMode::Before ? selected_fragment_index_
                                            : selected_fragment_index_ + 1;
}

std::optional<CompositionEditorViewModel::FragmentSpec>
CompositionEditorViewModel::buildFragmentFromEditor(
    const FragmentSpec::Kind kind, QString* error_message) const {
  switch (kind) {
    case FragmentSpec::Kind::BlockRef: {
      if (block_ref_block_id_.trimmed().isEmpty()) {
        *error_message = QStringLiteral("Block id is required.");
        return std::nullopt;
      }

      Version version;
      if (const auto parse_error = ParseVersionText(block_ref_version_, version);
          parse_error.has_value()) {
        *error_message = QString::fromStdString(*parse_error);
        return std::nullopt;
      }

      auto params = ParseParamsText(block_ref_params_);
      if (params.HasError()) {
        *error_message = QString::fromStdString(params.error().message);
        return std::nullopt;
      }

      return FragmentSpec{
          .kind = FragmentSpec::Kind::BlockRef,
          .block_id = block_ref_block_id_.trimmed(),
          .version = block_ref_version_.trimmed(),
          .params = block_ref_params_.trimmed(),
      };
    }
    case FragmentSpec::Kind::StaticText:
      if (static_text_.trimmed().isEmpty()) {
        *error_message = QStringLiteral("Text content is required.");
        return std::nullopt;
      }
      return FragmentSpec{
          .kind = FragmentSpec::Kind::StaticText,
          .text = static_text_,
      };
    case FragmentSpec::Kind::Separator:
      return FragmentSpec{
          .kind = FragmentSpec::Kind::Separator,
          .separator = separator_type_,
      };
  }

  *error_message = QStringLiteral("Unsupported fragment type.");
  return std::nullopt;
}

}  // namespace tf::gui
