#include "viewmodels/block_editor_view_model.h"

#include <QCoreApplication>
#include <QVariantMap>
#include <algorithm>
#include <ranges>
#include <sstream>
#include <string>

#include "app/session_view_model.h"
#include "models/blocks_model.h"
#include "prompt_constants.h"
#include "tf/block.h"
#include "tf/block_type.hpp"

namespace tf::gui {
namespace {

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

Result<Params> ParseDefaults(const QString& input) {
  Params out;
  std::istringstream stream(input.toStdString());
  std::string line;
  while (std::getline(stream, line)) {
    line = Trim(line);
    if (line.empty()) continue;
    const auto eq = line.find('=');
      if (eq == std::string::npos) {
        return Result<Params>(
          Error{ErrorCode::InvalidParamType,
                "Defaults must use one key=value pair per line"});
      }
    const std::string key = Trim(line.substr(0, eq));
    const std::string value = Trim(line.substr(eq + 1));
    if (key.empty()) {
      return Result<Params>(
          Error{ErrorCode::InvalidParamType, "Default parameter name cannot be empty"});
    }
    out[key] = value;
  }
  return Result<Params>(out);
}

std::vector<std::string> ParseTags(const QString& input) {
  std::vector<std::string> tags;
  std::string raw = input.toStdString();
  std::ranges::replace(raw, ',', '\n');
  std::istringstream stream(raw);
  std::string line;
  while (std::getline(stream, line)) {
    line = Trim(line);
    if (!line.empty()) {
      tags.push_back(line);
    }
  }
  std::sort(tags.begin(), tags.end());
  tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
  return tags;
}

BlockType ParseBlockType(const QString& value) {
  return BlockTypeFromString(value.toStdString());
}

Engine::VersionBump ParseBumpMode(const QString& value) {
  return value == QStringLiteral("Major") ? Engine::VersionBump::Major
                                          : Engine::VersionBump::Minor;
}

}  // namespace

BlockEditorViewModel::BlockEditorViewModel(SessionViewModel* session,
                                           BlocksModel* blocks, QObject* parent)
    : QObject(parent), session_(session), blocks_(blocks) {
  Q_ASSERT(session_ != nullptr);
  Q_ASSERT(blocks_ != nullptr);
  connect(session_, &SessionViewModel::engineReset, this,
          &BlockEditorViewModel::generationAvailabilityChanged);
}

BlockEditorViewModel* BlockEditorViewModel::create(QQmlEngine* qmlEngine,
                                                   QJSEngine* jsEngine) {
  Q_UNUSED(qmlEngine);
  Q_UNUSED(jsEngine);
  return instance();
}

BlockEditorViewModel* BlockEditorViewModel::instance() {
  static auto* singleton = new BlockEditorViewModel(
      SessionViewModel::instance(), BlocksModel::instance(),
      QCoreApplication::instance());
  return singleton;
}

bool BlockEditorViewModel::open() const { return !tabs_.empty(); }

QVariantList BlockEditorViewModel::tabEntries() const {
  QVariantList out;
  out.reserve(static_cast<qsizetype>(tabs_.size()));
  for (size_t i = 0; i < tabs_.size(); ++i) {
    const auto& tab = tabs_[i];
    QVariantMap entry;
    entry.insert(QStringLiteral("title"), tabTitle(tab));
    entry.insert(QStringLiteral("subtitle"), tabSubtitle(tab));
    entry.insert(QStringLiteral("dirty"), isDirty(tab));
    entry.insert(QStringLiteral("createMode"), tab.create_mode);
    entry.insert(QStringLiteral("blockId"), tab.block_id);
    entry.insert(QStringLiteral("currentVersion"), tab.current_version);
    entry.insert(QStringLiteral("current"), static_cast<int>(i) == current_tab_index_);
    out.push_back(entry);
  }
  return out;
}

int BlockEditorViewModel::currentTabIndex() const { return current_tab_index_; }

void BlockEditorViewModel::setCurrentTabIndex(int value) {
  if (value < 0 || value >= static_cast<int>(tabs_.size()) ||
      current_tab_index_ == value) {
    return;
  }
  activateTab(value);
}

bool BlockEditorViewModel::createMode() const {
  const auto* tab = currentTab();
  return tab != nullptr && tab->create_mode;
}

QString BlockEditorViewModel::dialogTitle() const {
  return createMode() ? QStringLiteral("Create Block")
                      : QStringLiteral("Edit Block");
}

QString BlockEditorViewModel::saveButtonText() const {
  return createMode() ? QStringLiteral("Create") : QStringLiteral("Save");
}

QString BlockEditorViewModel::blockId() const {
  const auto* tab = currentTab();
  return tab != nullptr ? tab->block_id : QString{};
}

QString BlockEditorViewModel::currentVersion() const {
  const auto* tab = currentTab();
  return tab != nullptr ? tab->current_version : QString{};
}

QString BlockEditorViewModel::type() const {
  const auto* tab = currentTab();
  return tab != nullptr ? tab->type : QStringLiteral("domain");
}

QString BlockEditorViewModel::language() const {
  const auto* tab = currentTab();
  return tab != nullptr ? tab->language : QStringLiteral("en");
}

QString BlockEditorViewModel::description() const {
  const auto* tab = currentTab();
  return tab != nullptr ? tab->description : QString{};
}

QString BlockEditorViewModel::revisionComment() const {
  const auto* tab = currentTab();
  return tab != nullptr ? tab->revision_comment : QString{};
}

QString BlockEditorViewModel::tagsText() const {
  const auto* tab = currentTab();
  return tab != nullptr ? tab->tags_text : QString{};
}

QString BlockEditorViewModel::defaultsText() const {
  const auto* tab = currentTab();
  return tab != nullptr ? tab->defaults_text : QString{};
}

QString BlockEditorViewModel::templateText() const {
  const auto* tab = currentTab();
  return tab != nullptr ? tab->template_text : QString{};
}

QString BlockEditorViewModel::aiPromptText() const {
  const auto* tab = currentTab();
  return tab != nullptr ? tab->ai_prompt_text : QString{};
}

QString BlockEditorViewModel::bumpMode() const {
  const auto* tab = currentTab();
  return tab != nullptr ? tab->bump_mode : QStringLiteral("Minor");
}

QStringList BlockEditorViewModel::typeOptions() const {
  return {QStringLiteral("role"), QStringLiteral("system"),
          QStringLiteral("mission"), QStringLiteral("safety"),
          QStringLiteral("constraint"), QStringLiteral("style"),
          QStringLiteral("domain"), QStringLiteral("meta")};
}

QStringList BlockEditorViewModel::bumpOptions() const {
  return {QStringLiteral("Minor"), QStringLiteral("Major")};
}

QString BlockEditorViewModel::statusText() const { return status_text_; }

bool BlockEditorViewModel::dirty() const {
  const auto* tab = currentTab();
  return tab != nullptr && isDirty(*tab);
}

bool BlockEditorViewModel::anyDirty() const {
  return std::ranges::any_of(tabs_,
                             [this](const EditorTab& tab) { return isDirty(tab); });
}

bool BlockEditorViewModel::saving() const { return saving_; }

bool BlockEditorViewModel::generating() const { return generating_; }

bool BlockEditorViewModel::aiGenerationAvailable() const {
  return session_->engine().HasBlockGenerator();
}

void BlockEditorViewModel::setBlockId(const QString& value) {
  auto* tab = currentTab();
  if (tab == nullptr || tab->block_id == value) return;
  tab->block_id = value;
  emit tabsChanged();
  emit formChanged();
}

void BlockEditorViewModel::setType(const QString& value) {
  auto* tab = currentTab();
  if (tab == nullptr || tab->type == value) return;
  tab->type = value;
  emit tabsChanged();
  emit formChanged();
}

void BlockEditorViewModel::setLanguage(const QString& value) {
  auto* tab = currentTab();
  if (tab == nullptr || tab->language == value) return;
  tab->language = value;
  emit tabsChanged();
  emit formChanged();
}

void BlockEditorViewModel::setDescription(const QString& value) {
  auto* tab = currentTab();
  if (tab == nullptr || tab->description == value) return;
  tab->description = value;
  emit tabsChanged();
  emit formChanged();
}

void BlockEditorViewModel::setRevisionComment(const QString& value) {
  auto* tab = currentTab();
  if (tab == nullptr || tab->revision_comment == value) return;
  tab->revision_comment = value;
  emit tabsChanged();
  emit formChanged();
}

void BlockEditorViewModel::setTagsText(const QString& value) {
  auto* tab = currentTab();
  if (tab == nullptr || tab->tags_text == value) return;
  tab->tags_text = value;
  emit tabsChanged();
  emit formChanged();
}

void BlockEditorViewModel::setDefaultsText(const QString& value) {
  auto* tab = currentTab();
  if (tab == nullptr || tab->defaults_text == value) return;
  tab->defaults_text = value;
  emit tabsChanged();
  emit formChanged();
}

void BlockEditorViewModel::setTemplateText(const QString& value) {
  auto* tab = currentTab();
  if (tab == nullptr || tab->template_text == value) return;
  tab->template_text = value;
  emit tabsChanged();
  emit formChanged();
}

void BlockEditorViewModel::setAiPromptText(const QString& value) {
  auto* tab = currentTab();
  if (tab == nullptr || tab->ai_prompt_text == value) return;
  tab->ai_prompt_text = value;
  emit tabsChanged();
  emit formChanged();
}

void BlockEditorViewModel::setBumpMode(const QString& value) {
  auto* tab = currentTab();
  if (tab == nullptr || tab->bump_mode == value) return;
  tab->bump_mode = value;
  emit tabsChanged();
  emit formChanged();
}

void BlockEditorViewModel::openEditor() {
  auto loaded_tab = loadSelectedBlockTab();
  if (!loaded_tab.has_value()) return;

  const int existing_index =
      findExistingTab(loaded_tab->block_id, loaded_tab->current_version);
  if (existing_index >= 0) {
    activateTab(existing_index);
    return;
  }

  const bool was_open = open();
  tabs_.push_back(std::move(*loaded_tab));
  current_tab_index_ = static_cast<int>(tabs_.size()) - 1;
  emit tabsChanged();
  emitCurrentTabChanged();
  if (!was_open) emit openChanged();
}

void BlockEditorViewModel::openCreateEditor() {
  const int existing_create_index = findCreateTab();
  if (existing_create_index >= 0) {
    activateTab(existing_create_index);
    return;
  }

  const bool was_open = open();
  EditorTab tab = MakeDefaultTab();
  tab.create_mode = true;
  tab.original_state_key = currentStateKey(tab);
  tabs_.push_back(std::move(tab));
  current_tab_index_ = static_cast<int>(tabs_.size()) - 1;
  setStatusText(QStringLiteral("Enter block data and press Create."));
  emit tabsChanged();
  emitCurrentTabChanged();
  if (!was_open) emit openChanged();
}

void BlockEditorViewModel::closeEditor() {
  closeTab(current_tab_index_);
}

void BlockEditorViewModel::closeTab(int index) {
  if (index < 0 || index >= static_cast<int>(tabs_.size())) return;
  const bool was_open = open();
  tabs_.erase(tabs_.begin() + index);
  if (tabs_.empty()) {
    current_tab_index_ = -1;
    emit tabsChanged();
    emitCurrentTabChanged();
    if (was_open) emit openChanged();
    return;
  }

  if (current_tab_index_ >= static_cast<int>(tabs_.size())) {
    current_tab_index_ = static_cast<int>(tabs_.size()) - 1;
  } else if (index < current_tab_index_) {
    --current_tab_index_;
  } else if (index == current_tab_index_) {
    current_tab_index_ = std::min(index, static_cast<int>(tabs_.size()) - 1);
  }

  emit tabsChanged();
  emitCurrentTabChanged();
}

void BlockEditorViewModel::closeAllEditors() {
  if (!open()) return;
  tabs_.clear();
  current_tab_index_ = -1;
  emit tabsChanged();
  emitCurrentTabChanged();
  emit openChanged();
}

void BlockEditorViewModel::save() {
  auto* tab = currentTab();
  if (tab == nullptr) return;

  if (tab->block_id.trimmed().isEmpty()) {
    setStatusText(QStringLiteral("Block id is required."));
    return;
  }

  auto defaults_result = ParseDefaults(tab->defaults_text);
  if (defaults_result.HasError()) {
    setStatusText(QString::fromStdString(defaults_result.error().message));
    return;
  }

  saving_ = true;
  emit savingChanged();

  BlockDraftBuilder builder(tab->block_id.toStdString());
  builder.WithType(ParseBlockType(tab->type))
      .WithLanguage(tab->language.toStdString())
      .WithDescription(tab->description.toStdString())
      .WithRevisionComment(tab->revision_comment.trimmed().toStdString())
      .WithTemplate(Template(tab->template_text.toStdString()))
      .WithDefaults(defaults_result.value());

  for (const auto& tag : ParseTags(tab->tags_text)) {
    builder.WithTag(tag);
  }

  auto result = tab->create_mode
                    ? session_->engine().PublishBlock(builder.build(),
                                                     ParseBumpMode(tab->bump_mode))
                    : session_->engine().UpdateBlock(builder.build(),
                                                    ParseBumpMode(tab->bump_mode));

  saving_ = false;
  emit savingChanged();

  if (result.HasError()) {
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(result.error().message)));
    return;
  }

  if (!blocks_->syncLatestBlockNode(tab->block_id)) {
    blocks_->reload();
  }
  blocks_->selectBlock(tab->block_id);
  blocks_->selectLatestVersion();
  setStatusText(QString("%1 %2 as version %3")
                    .arg(tab->create_mode ? QStringLiteral("Created")
                                      : QStringLiteral("Saved"),
                         tab->block_id,
                         QString::fromStdString(result.value().version().ToString())));
  tab->current_version = QString::fromStdString(result.value().version().ToString());
  tab->create_mode = false;
  tab->revision_comment.clear();
  tab->ai_prompt_text.clear();
  tab->original_state_key = currentStateKey(*tab);
  emit tabsChanged();
  emitCurrentTabChanged();
  emit saved();
}

void BlockEditorViewModel::generate() {
  auto* tab = currentTab();
  if (tab == nullptr) return;
  if (!session_->engine().HasBlockGenerator()) {
    setStatusText(QStringLiteral("Configure AI settings first."));
    return;
  }
  if (tab->ai_prompt_text.trimmed().isEmpty()) {
    setStatusText(QStringLiteral("AI prompt is required."));
    return;
  }

  generating_ = true;
  emit generatingChanged();

  BlockGenerationRequest request{
      .prompt = tab->ai_prompt_text.trimmed().toStdString(),
      .allow_id_collision = false,
  };
  if (!tab->block_id.trimmed().isEmpty()) {
    request.preferred_id = tab->block_id.trimmed().toStdString();
  }
  if (!tab->language.trimmed().isEmpty()) {
    request.preferred_language = tab->language.trimmed().toStdString();
  }
  request.preferred_type = ParseBlockType(tab->type);
  request.existing_block_ids = session_->engine().ListBlocks();

  auto result = session_->engine().GenerateBlockData(request);

  generating_ = false;
  emit generatingChanged();

  if (result.HasError()) {
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(result.error().message)));
    return;
  }

  const auto& generated = result.value();
  tab->block_id = QString::fromStdString(generated.id);
  tab->type = QString::fromUtf8(BlockTypeToString(generated.type).data(),
                            static_cast<int>(BlockTypeToString(generated.type).size()));
  tab->language = QString::fromStdString(generated.language);
  tab->description = QString::fromStdString(generated.description);
  tab->template_text = QString::fromStdString(generated.templ);

  QStringList tags;
  for (const auto& tag : generated.tags) {
    tags.push_back(QString::fromStdString(tag));
  }
  std::sort(tags.begin(), tags.end());
  tab->tags_text = tags.join(QStringLiteral("\n"));

  QStringList defaults;
  for (const auto& [key, value] : generated.defaults) {
    defaults.push_back(QString::fromStdString(key + "=" + value));
  }
  std::sort(defaults.begin(), defaults.end());
  tab->defaults_text = defaults.join(QStringLiteral("\n"));

  setStatusText(QStringLiteral("AI suggestion loaded into the form."));
  emit tabsChanged();
  emitCurrentTabChanged();
  emit formChanged();
}

void BlockEditorViewModel::revise() {
  auto* tab = currentTab();
  if (tab == nullptr) return;
  if (tab->create_mode) {
    setStatusText(QStringLiteral("Revise is available only for existing blocks."));
    return;
  }
  if (!session_->engine().HasBlockGenerator()) {
    setStatusText(QStringLiteral("Configure AI settings first."));
    return;
  }
  if (tab->ai_prompt_text.trimmed().isEmpty()) {
    setStatusText(QStringLiteral("AI instruction is required."));
    return;
  }

  generating_ = true;
  emit generatingChanged();

  QString revision_prompt;
  revision_prompt +=
      QString::fromUtf8(tf::ai::prompts::kReviseBlockIntro.data(),
                        static_cast<qsizetype>(
                            tf::ai::prompts::kReviseBlockIntro.size()));
  revision_prompt += QString::fromUtf8(
      tf::ai::prompts::kReviseBlockIdentityGuidance.data(),
      static_cast<qsizetype>(
          tf::ai::prompts::kReviseBlockIdentityGuidance.size()));
  revision_prompt += QString::fromUtf8(
      tf::ai::prompts::kReviseBlockPreservationGuidance.data(),
      static_cast<qsizetype>(
          tf::ai::prompts::kReviseBlockPreservationGuidance.size()));
  revision_prompt += QString::fromUtf8(
      tf::ai::prompts::kReviseBlockUserInstructionLabel.data(),
      static_cast<qsizetype>(
          tf::ai::prompts::kReviseBlockUserInstructionLabel.size()));
  revision_prompt += tab->ai_prompt_text.trimmed();
  revision_prompt += QString::fromUtf8(
      tf::ai::prompts::kReviseBlockCurrentBlockLabel.data(),
      static_cast<qsizetype>(
          tf::ai::prompts::kReviseBlockCurrentBlockLabel.size()));
  revision_prompt += QStringLiteral("Id: ") + tab->block_id + QStringLiteral("\n");
  revision_prompt += QStringLiteral("Type: ") + tab->type + QStringLiteral("\n");
  revision_prompt +=
      QStringLiteral("Language: ") + tab->language + QStringLiteral("\n");
  revision_prompt +=
      QStringLiteral("Description:\n") + tab->description + QStringLiteral("\n\n");
  revision_prompt += QStringLiteral("Tags:\n") + tab->tags_text +
                     QStringLiteral("\n\n");
  revision_prompt += QStringLiteral("Defaults:\n") + tab->defaults_text +
                     QStringLiteral("\n\n");
  revision_prompt += QStringLiteral("Template:\n") + tab->template_text +
                     QStringLiteral("\n");

  BlockGenerationRequest request{
      .prompt = revision_prompt.toStdString(),
      .preferred_id = tab->block_id.toStdString(),
      .preferred_type = ParseBlockType(tab->type),
      .preferred_language = tab->language.toStdString(),
      .existing_block_ids = session_->engine().ListBlocks(),
      .allow_id_collision = true,
  };

  auto result = session_->engine().GenerateBlockData(request);

  generating_ = false;
  emit generatingChanged();

  if (result.HasError()) {
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(result.error().message)));
    return;
  }

  const auto& generated = result.value();
  tab->description = QString::fromStdString(generated.description);
  tab->template_text = QString::fromStdString(generated.templ);

  QStringList tags;
  for (const auto& tag : generated.tags) {
    tags.push_back(QString::fromStdString(tag));
  }
  std::sort(tags.begin(), tags.end());
  if (!tags.isEmpty()) {
    tab->tags_text = tags.join(QStringLiteral("\n"));
  }

  QStringList defaults;
  for (const auto& [key, value] : generated.defaults) {
    defaults.push_back(QString::fromStdString(key + "=" + value));
  }
  std::sort(defaults.begin(), defaults.end());
  if (!defaults.isEmpty()) {
    tab->defaults_text = defaults.join(QStringLiteral("\n"));
  }

  emit tabsChanged();
  emitCurrentTabChanged();
  setStatusText(QStringLiteral("AI revision loaded into the form. Review and save a new version."));
}

void BlockEditorViewModel::setStatusText(QString value) {
  if (status_text_ == value) return;
  status_text_ = std::move(value);
  session_->publishStatus(status_text_);
  emit statusTextChanged();
}

BlockEditorViewModel::EditorTab BlockEditorViewModel::MakeDefaultTab() {
  return EditorTab{};
}

std::optional<BlockEditorViewModel::EditorTab> BlockEditorViewModel::loadSelectedBlockTab() {
  const QString selected_id = blocks_->selectedBlockId();
  if (selected_id.isEmpty()) {
    setStatusText(QStringLiteral("Select a block first."));
    return std::nullopt;
  }

  std::optional<Version> version;
  const QString selected_version = blocks_->selectedBlockVersion().trimmed();
  if (!selected_version.isEmpty()) {
    const auto parts = selected_version.split('.');
    bool major_ok = false;
    bool minor_ok = false;
    if (parts.size() == 2) {
      const int major = parts[0].toInt(&major_ok);
      const int minor = parts[1].toInt(&minor_ok);
      if (major_ok && minor_ok && major >= 0 && major <= 65535 && minor >= 0 &&
          minor <= 65535) {
        version = Version{static_cast<uint16_t>(major),
                          static_cast<uint16_t>(minor)};
      }
    }
    if (!version.has_value()) {
      setStatusText(QStringLiteral("Selected block version is invalid."));
      return std::nullopt;
    }
  }

  auto result = session_->engine().LoadBlock(selected_id.toStdString(), version);
  if (result.HasError()) {
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(result.error().message)));
    return std::nullopt;
  }

  const auto& block = result.value();
  EditorTab tab;
  tab.block_id = selected_id;
  tab.current_version = QString::fromStdString(block.version().ToString());
  tab.type = QString::fromUtf8(BlockTypeToString(block.type()).data(),
                            static_cast<int>(BlockTypeToString(block.type()).size()));
  tab.language = QString::fromStdString(block.language());
  tab.description = QString::fromStdString(block.description());
  tab.revision_comment = QString::fromStdString(block.revision_comment());
  tab.template_text = QString::fromStdString(block.templ().Content());

  QStringList tags;
  for (const auto& tag : block.tags()) {
    tags.push_back(QString::fromStdString(tag));
  }
  std::sort(tags.begin(), tags.end());
  tab.tags_text = tags.join(QStringLiteral("\n"));

  QStringList defaults;
  for (const auto& [key, value] : block.defaults()) {
    defaults.push_back(QString::fromStdString(key + "=" + value));
  }
  std::sort(defaults.begin(), defaults.end());
  tab.defaults_text = defaults.join(QStringLiteral("\n"));
  tab.bump_mode = QStringLiteral("Minor");
  tab.create_mode = false;
  tab.original_state_key = currentStateKey(tab);
  setStatusText(QStringLiteral("Editing selected block."));
  return tab;
}

BlockEditorViewModel::EditorTab* BlockEditorViewModel::currentTab() {
  if (current_tab_index_ < 0 || current_tab_index_ >= static_cast<int>(tabs_.size())) {
    return nullptr;
  }
  return &tabs_[current_tab_index_];
}

const BlockEditorViewModel::EditorTab* BlockEditorViewModel::currentTab() const {
  if (current_tab_index_ < 0 || current_tab_index_ >= static_cast<int>(tabs_.size())) {
    return nullptr;
  }
  return &tabs_[current_tab_index_];
}

QString BlockEditorViewModel::currentStateKey(const EditorTab& tab) const {
  return tab.block_id + QStringLiteral("\n") + tab.current_version +
         QStringLiteral("\n") + tab.type + QStringLiteral("\n") + tab.language +
         QStringLiteral("\n") + tab.description + QStringLiteral("\n") +
         tab.revision_comment + QStringLiteral("\n") + tab.tags_text +
         QStringLiteral("\n") + tab.defaults_text + QStringLiteral("\n") +
         tab.template_text + QStringLiteral("\n") + tab.ai_prompt_text +
         QStringLiteral("\n") + tab.bump_mode;
}

bool BlockEditorViewModel::isDirty(const EditorTab& tab) const {
  return currentStateKey(tab) != tab.original_state_key;
}

QString BlockEditorViewModel::tabTitle(const EditorTab& tab) const {
  if (tab.create_mode) {
    return tab.block_id.trimmed().isEmpty() ? QStringLiteral("New Block")
                                            : tab.block_id;
  }
  return tab.block_id.trimmed().isEmpty() ? QStringLiteral("Edit") : tab.block_id;
}

QString BlockEditorViewModel::tabSubtitle(const EditorTab& tab) const {
  if (tab.create_mode) return QStringLiteral("draft");
  return tab.current_version.trimmed().isEmpty() ? QStringLiteral("draft")
                                                 : QStringLiteral("v") + tab.current_version;
}

void BlockEditorViewModel::activateTab(int index) {
  if (index < 0 || index >= static_cast<int>(tabs_.size())) return;
  if (current_tab_index_ == index) return;
  current_tab_index_ = index;
  emit tabsChanged();
  emitCurrentTabChanged();
}

void BlockEditorViewModel::emitCurrentTabChanged() {
  emit blockLoaded();
  emit formChanged();
}

int BlockEditorViewModel::findExistingTab(const QString& blockId,
                                          const QString& version) const {
  for (size_t i = 0; i < tabs_.size(); ++i) {
    const auto& tab = tabs_[i];
    if (!tab.create_mode && tab.block_id == blockId &&
        tab.current_version == version) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

int BlockEditorViewModel::findCreateTab() const {
  for (size_t i = 0; i < tabs_.size(); ++i) {
    if (tabs_[i].create_mode) return static_cast<int>(i);
  }
  return -1;
}

}  // namespace tf::gui
