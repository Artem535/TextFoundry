#include "viewmodels/block_editor_view_model.h"

#include <QCoreApplication>
#include <algorithm>
#include <ranges>
#include <sstream>
#include <string>

#include "app/session_view_model.h"
#include "models/blocks_model.h"
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

bool BlockEditorViewModel::open() const { return open_; }

bool BlockEditorViewModel::createMode() const { return create_mode_; }

QString BlockEditorViewModel::dialogTitle() const {
  return create_mode_ ? QStringLiteral("Create Block")
                      : QStringLiteral("Edit Block");
}

QString BlockEditorViewModel::saveButtonText() const {
  return create_mode_ ? QStringLiteral("Create") : QStringLiteral("Save");
}

QString BlockEditorViewModel::blockId() const { return block_id_; }

QString BlockEditorViewModel::currentVersion() const { return current_version_; }

QString BlockEditorViewModel::type() const { return type_; }

QString BlockEditorViewModel::language() const { return language_; }

QString BlockEditorViewModel::description() const { return description_; }

QString BlockEditorViewModel::revisionComment() const { return revision_comment_; }

QString BlockEditorViewModel::tagsText() const { return tags_text_; }

QString BlockEditorViewModel::defaultsText() const { return defaults_text_; }

QString BlockEditorViewModel::templateText() const { return template_text_; }

QString BlockEditorViewModel::aiPromptText() const { return ai_prompt_text_; }

QString BlockEditorViewModel::bumpMode() const { return bump_mode_; }

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

bool BlockEditorViewModel::saving() const { return saving_; }

bool BlockEditorViewModel::generating() const { return generating_; }

bool BlockEditorViewModel::aiGenerationAvailable() const {
  return session_->engine().HasBlockGenerator();
}

void BlockEditorViewModel::setBlockId(const QString& value) {
  if (block_id_ == value) return;
  block_id_ = value;
  emit formChanged();
}

void BlockEditorViewModel::setType(const QString& value) {
  if (type_ == value) return;
  type_ = value;
  emit formChanged();
}

void BlockEditorViewModel::setLanguage(const QString& value) {
  if (language_ == value) return;
  language_ = value;
  emit formChanged();
}

void BlockEditorViewModel::setDescription(const QString& value) {
  if (description_ == value) return;
  description_ = value;
  emit formChanged();
}

void BlockEditorViewModel::setRevisionComment(const QString& value) {
  if (revision_comment_ == value) return;
  revision_comment_ = value;
  emit formChanged();
}

void BlockEditorViewModel::setTagsText(const QString& value) {
  if (tags_text_ == value) return;
  tags_text_ = value;
  emit formChanged();
}

void BlockEditorViewModel::setDefaultsText(const QString& value) {
  if (defaults_text_ == value) return;
  defaults_text_ = value;
  emit formChanged();
}

void BlockEditorViewModel::setTemplateText(const QString& value) {
  if (template_text_ == value) return;
  template_text_ = value;
  emit formChanged();
}

void BlockEditorViewModel::setAiPromptText(const QString& value) {
  if (ai_prompt_text_ == value) return;
  ai_prompt_text_ = value;
  emit formChanged();
}

void BlockEditorViewModel::setBumpMode(const QString& value) {
  if (bump_mode_ == value) return;
  bump_mode_ = value;
  emit formChanged();
}

void BlockEditorViewModel::openEditor() {
  if (!loadSelectedBlock()) return;
  create_mode_ = false;
  open_ = true;
  emit blockLoaded();
  emit openChanged();
}

void BlockEditorViewModel::openCreateEditor() {
  resetForm();
  create_mode_ = true;
  open_ = true;
  setStatusText(QStringLiteral("Enter block data and press Create."));
  emit blockLoaded();
  emit formChanged();
  emit openChanged();
}

void BlockEditorViewModel::closeEditor() {
  if (!open_) return;
  open_ = false;
  emit openChanged();
}

void BlockEditorViewModel::save() {
  if (block_id_.trimmed().isEmpty()) {
    setStatusText(QStringLiteral("Block id is required."));
    return;
  }

  auto defaults_result = ParseDefaults(defaults_text_);
  if (defaults_result.HasError()) {
    setStatusText(QString::fromStdString(defaults_result.error().message));
    return;
  }

  saving_ = true;
  emit savingChanged();

  BlockDraftBuilder builder(block_id_.toStdString());
  builder.WithType(ParseBlockType(type_))
      .WithLanguage(language_.toStdString())
      .WithDescription(description_.toStdString())
      .WithRevisionComment(revision_comment_.trimmed().toStdString())
      .WithTemplate(Template(template_text_.toStdString()))
      .WithDefaults(defaults_result.value());

  for (const auto& tag : ParseTags(tags_text_)) {
    builder.WithTag(tag);
  }

  auto result = create_mode_
                    ? session_->engine().PublishBlock(builder.build(),
                                                     ParseBumpMode(bump_mode_))
                    : session_->engine().UpdateBlock(builder.build(),
                                                    ParseBumpMode(bump_mode_));

  saving_ = false;
  emit savingChanged();

  if (result.HasError()) {
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(result.error().message)));
    return;
  }

  blocks_->reload();
  blocks_->selectBlock(block_id_);
  blocks_->selectLatestVersion();
  setStatusText(QString("%1 %2 as version %3")
                    .arg(create_mode_ ? QStringLiteral("Created")
                                      : QStringLiteral("Saved"),
                         block_id_,
                         QString::fromStdString(result.value().version().ToString())));
  emit saved();
  closeEditor();
}

void BlockEditorViewModel::generate() {
  if (!session_->engine().HasBlockGenerator()) {
    setStatusText(QStringLiteral("Configure AI settings first."));
    return;
  }
  if (ai_prompt_text_.trimmed().isEmpty()) {
    setStatusText(QStringLiteral("AI prompt is required."));
    return;
  }

  generating_ = true;
  emit generatingChanged();

  BlockGenerationRequest request{
      .prompt = ai_prompt_text_.trimmed().toStdString(),
      .allow_id_collision = false,
  };
  if (!block_id_.trimmed().isEmpty()) {
    request.preferred_id = block_id_.trimmed().toStdString();
  }
  if (!language_.trimmed().isEmpty()) {
    request.preferred_language = language_.trimmed().toStdString();
  }
  request.preferred_type = ParseBlockType(type_);
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
  block_id_ = QString::fromStdString(generated.id);
  type_ = QString::fromUtf8(BlockTypeToString(generated.type).data(),
                            static_cast<int>(BlockTypeToString(generated.type).size()));
  language_ = QString::fromStdString(generated.language);
  description_ = QString::fromStdString(generated.description);
  template_text_ = QString::fromStdString(generated.templ);

  QStringList tags;
  for (const auto& tag : generated.tags) {
    tags.push_back(QString::fromStdString(tag));
  }
  std::sort(tags.begin(), tags.end());
  tags_text_ = tags.join(QStringLiteral("\n"));

  QStringList defaults;
  for (const auto& [key, value] : generated.defaults) {
    defaults.push_back(QString::fromStdString(key + "=" + value));
  }
  std::sort(defaults.begin(), defaults.end());
  defaults_text_ = defaults.join(QStringLiteral("\n"));

  setStatusText(QStringLiteral("AI suggestion loaded into the form."));
  emit blockLoaded();
  emit formChanged();
}

void BlockEditorViewModel::setStatusText(QString value) {
  if (status_text_ == value) return;
  status_text_ = std::move(value);
  emit statusTextChanged();
}

void BlockEditorViewModel::resetForm() {
  block_id_.clear();
  current_version_.clear();
  type_ = QStringLiteral("domain");
  language_ = QStringLiteral("en");
  description_.clear();
  revision_comment_.clear();
  tags_text_.clear();
  defaults_text_.clear();
  template_text_.clear();
  ai_prompt_text_.clear();
  bump_mode_ = QStringLiteral("Minor");
}

bool BlockEditorViewModel::loadSelectedBlock() {
  const QString selected_id = blocks_->selectedBlockId();
  if (selected_id.isEmpty()) {
    setStatusText(QStringLiteral("Select a block first."));
    return false;
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
      return false;
    }
  }

  auto result = session_->engine().LoadBlock(selected_id.toStdString(), version);
  if (result.HasError()) {
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(result.error().message)));
    return false;
  }

  const auto& block = result.value();
  block_id_ = selected_id;
  current_version_ = QString::fromStdString(block.version().ToString());
  type_ = QString::fromUtf8(BlockTypeToString(block.type()).data(),
                            static_cast<int>(BlockTypeToString(block.type()).size()));
  language_ = QString::fromStdString(block.language());
  description_ = QString::fromStdString(block.description());
  revision_comment_ = QString::fromStdString(block.revision_comment());
  template_text_ = QString::fromStdString(block.templ().Content());

  QStringList tags;
  for (const auto& tag : block.tags()) {
    tags.push_back(QString::fromStdString(tag));
  }
  std::sort(tags.begin(), tags.end());
  tags_text_ = tags.join(QStringLiteral("\n"));

  QStringList defaults;
  for (const auto& [key, value] : block.defaults()) {
    defaults.push_back(QString::fromStdString(key + "=" + value));
  }
  std::sort(defaults.begin(), defaults.end());
  defaults_text_ = defaults.join(QStringLiteral("\n"));

  bump_mode_ = QStringLiteral("Minor");
  create_mode_ = false;
  setStatusText(QStringLiteral("Editing selected block."));
  emit blockLoaded();
  emit formChanged();
  return true;
}

}  // namespace tf::gui
