#include "viewmodels/block_slice_view_model.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <algorithm>
#include <optional>
#include <unordered_set>

#include "app/session_view_model.h"
#include "models/blocks_model.h"
#include "tf/block.h"
#include "tf/block_type.hpp"
#include "tf/composition.h"
#include "viewmodels/compositions_view_model.h"

namespace tf::gui {
namespace {

QString BlockTypeLabel(BlockType type) {
  const auto value = BlockTypeToString(type);
  return QString::fromUtf8(value.data(), static_cast<int>(value.size()));
}

QString Slugify(QString value) {
  value = value.trimmed().toLower();
  value.replace(QRegularExpression(QStringLiteral("[^a-z0-9]+")),
                QStringLiteral("_"));
  value.remove(QRegularExpression(QStringLiteral("^_+|_+$")));
  return value.isEmpty() ? QStringLiteral("section") : value;
}

BlockType GuessSectionType(const QString& section_name) {
  const QString key = Slugify(section_name);
  if (key == QStringLiteral("system")) return BlockType::System;
  if (key == QStringLiteral("mission") || key == QStringLiteral("goals") ||
      key == QStringLiteral("objective") ||
      key == QStringLiteral("objectives")) {
    return BlockType::Mission;
  }
  if (key == QStringLiteral("safety") || key == QStringLiteral("guardrails")) {
    return BlockType::Safety;
  }
  if (key == QStringLiteral("behavior") || key == QStringLiteral("style") ||
      key == QStringLiteral("response_flow") ||
      key == QStringLiteral("depth_guidelines")) {
    return BlockType::Style;
  }
  if (key == QStringLiteral("final_instructions")) return BlockType::Constraint;
  if (key == QStringLiteral("history") || key == QStringLiteral("examples")) {
    return BlockType::Meta;
  }
  return BlockType::Domain;
}

QString BuildSectionDescription(const QString& section_name) {
  const QString key = Slugify(section_name);
  if (key == QStringLiteral("system")) {
    return QStringLiteral("System-level framing and operating instructions.");
  }
  if (key == QStringLiteral("mission")) {
    return QStringLiteral("Mission goals and intended responsibilities.");
  }
  if (key == QStringLiteral("safety")) {
    return QStringLiteral("Safety rules, prohibitions, and escalation guidance.");
  }
  if (key == QStringLiteral("behavior")) {
    return QStringLiteral("Behavioral principles for interaction.");
  }
  if (key == QStringLiteral("response_flow")) {
    return QStringLiteral("Required response structure and sequencing.");
  }
  if (key == QStringLiteral("depth_guidelines")) {
    return QStringLiteral("Guidance for response length and level of detail.");
  }
  if (key == QStringLiteral("style")) {
    return QStringLiteral("Tone and expression guidelines.");
  }
  if (key == QStringLiteral("examples")) {
    return QStringLiteral("In-context examples to preserve behavior.");
  }
  if (key == QStringLiteral("history")) {
    return QStringLiteral("Conversation history placeholder.");
  }
  if (key == QStringLiteral("final_instructions")) {
    return QStringLiteral("Final hard constraints for every response.");
  }
  return QStringLiteral("Extracted from the %1 section.").arg(section_name);
}

QString UniqueBlockId(QString base_id, std::unordered_set<std::string>& used_ids) {
  QString candidate = std::move(base_id);
  int suffix = 2;
  while (used_ids.contains(candidate.toStdString())) {
    candidate = QStringLiteral("%1_%2").arg(base_id).arg(suffix++);
  }
  used_ids.insert(candidate.toStdString());
  return candidate;
}

QString UniqueCompositionId(QString base_id, const QStringList& existing_ids) {
  QString candidate = std::move(base_id);
  int suffix = 2;
  while (existing_ids.contains(candidate)) {
    candidate = QStringLiteral("%1_%2").arg(base_id).arg(suffix++);
  }
  return candidate;
}

std::optional<std::vector<GeneratedBlockData>> ParseTaggedSections(
    const QString& source_text, const QString& language,
    const QString& namespace_prefix, const std::vector<BlockId>& existing_ids) {
  static const QRegularExpression kSectionPattern(
      QStringLiteral("<([A-Za-z_][A-Za-z0-9_]*)>\\s*([\\s\\S]*?)\\s*</\\1>"));

  std::unordered_set<std::string> used_ids(existing_ids.begin(), existing_ids.end());
  std::vector<GeneratedBlockData> blocks;
  auto it = kSectionPattern.globalMatch(source_text);
  while (it.hasNext()) {
    const auto match = it.next();
    const QString section_name = match.captured(1).trimmed();
    const QString section_body = match.captured(2).trimmed();
    if (section_name.isEmpty() || section_body.isEmpty()) continue;

    const QString base_name = Slugify(section_name);
    const QString base_id = namespace_prefix.trimmed().isEmpty()
                                ? base_name
                                : QStringLiteral("%1.%2")
                                      .arg(namespace_prefix.trimmed(), base_name);

    blocks.push_back(GeneratedBlockData{
        .id = UniqueBlockId(base_id, used_ids).toStdString(),
        .type = GuessSectionType(section_name),
        .language = language.trimmed().isEmpty() ? std::string("en")
                                                 : language.trimmed().toStdString(),
        .description = BuildSectionDescription(section_name).toStdString(),
        .templ = QStringLiteral("<%1>\n%2\n</%1>")
                     .arg(section_name, section_body)
                     .toStdString(),
        .defaults = {},
        .tags = {base_name.toStdString(), "section"},
    });
  }

  if (blocks.size() < 2) return std::nullopt;
  return blocks;
}

}  // namespace

BlockSliceViewModel::BlockSliceViewModel(SessionViewModel* session,
                                         BlocksModel* blocks, QObject* parent)
    : QObject(parent), session_(session), blocks_(blocks) {
  Q_ASSERT(session_ != nullptr);
  Q_ASSERT(blocks_ != nullptr);
  connect(session_, &SessionViewModel::engineReset, this,
          &BlockSliceViewModel::generationAvailabilityChanged);
}

BlockSliceViewModel* BlockSliceViewModel::create(QQmlEngine* qmlEngine,
                                                 QJSEngine* jsEngine) {
  Q_UNUSED(qmlEngine);
  Q_UNUSED(jsEngine);
  return instance();
}

BlockSliceViewModel* BlockSliceViewModel::instance() {
  static auto* singleton = new BlockSliceViewModel(
      SessionViewModel::instance(), BlocksModel::instance(),
      QCoreApplication::instance());
  return singleton;
}

bool BlockSliceViewModel::open() const { return open_; }

QString BlockSliceViewModel::sourcePromptText() const { return source_prompt_text_; }

QString BlockSliceViewModel::namespacePrefix() const { return namespace_prefix_; }

QString BlockSliceViewModel::language() const { return language_; }

QString BlockSliceViewModel::compositionPreviewId() const {
  return BuildCompositionPreviewId();
}

QString BlockSliceViewModel::compositionPreviewText() const {
  return BuildCompositionPreviewText();
}

QString BlockSliceViewModel::generatedPreviewText() const {
  return BuildGeneratedPreview();
}

int BlockSliceViewModel::generatedCount() const {
  return static_cast<int>(generated_blocks_.size());
}

QString BlockSliceViewModel::statusText() const { return status_text_; }

bool BlockSliceViewModel::generating() const { return generating_; }

bool BlockSliceViewModel::publishing() const { return publishing_; }

bool BlockSliceViewModel::aiGenerationAvailable() const {
  return session_->engine().HasBlockGenerator();
}

void BlockSliceViewModel::setSourcePromptText(const QString& value) {
  if (source_prompt_text_ == value) return;
  source_prompt_text_ = value;
  emit formChanged();
}

void BlockSliceViewModel::setNamespacePrefix(const QString& value) {
  if (namespace_prefix_ == value) return;
  namespace_prefix_ = value;
  emit formChanged();
}

void BlockSliceViewModel::setLanguage(const QString& value) {
  if (language_ == value) return;
  language_ = value;
  emit formChanged();
}

void BlockSliceViewModel::openDialog() {
  resetForm();
  open_ = true;
  setStatusText(QStringLiteral(
      "Paste prompt text, generate block suggestions, then publish the batch."));
  emit generatedChanged();
  emit openChanged();
}

void BlockSliceViewModel::closeDialog() {
  if (!open_) return;
  open_ = false;
  emit openChanged();
}

void BlockSliceViewModel::generate() {
  if (source_prompt_text_.trimmed().isEmpty()) {
    setStatusText(QStringLiteral("Source prompt is required."));
    return;
  }

  generating_ = true;
  emit generatingChanged();

  const auto existing_ids = session_->engine().ListBlocks();
  if (auto parsed = ParseTaggedSections(source_prompt_text_, language_,
                                        namespace_prefix_, existing_ids);
      parsed.has_value()) {
    generated_blocks_ = std::move(*parsed);
    generating_ = false;
    emit generatingChanged();
    emit generatedChanged();
    setStatusText(QStringLiteral(
                      "Parsed %1 tagged sections directly without AI rewriting.")
                      .arg(generated_blocks_.size()));
    return;
  }

  if (!session_->engine().HasBlockGenerator()) {
    generating_ = false;
    emit generatingChanged();
    setStatusText(QStringLiteral("Configure AI settings first."));
    return;
  }

  PromptSlicingRequest request{
      .source_text = source_prompt_text_.trimmed().toStdString(),
      .existing_block_ids = existing_ids,
      .allow_id_collision = false,
  };
  if (!language_.trimmed().isEmpty()) {
    request.preferred_language = language_.trimmed().toStdString();
  }
  if (!namespace_prefix_.trimmed().isEmpty()) {
    request.namespace_prefix = namespace_prefix_.trimmed().toStdString();
  }

  auto result = session_->engine().GenerateBlockBatchData(request);

  generating_ = false;
  emit generatingChanged();

  if (result.HasError()) {
    generated_blocks_.clear();
    emit generatedChanged();
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(result.error().message)));
    return;
  }

  generated_blocks_ = std::move(result).value().blocks;
  emit generatedChanged();
  setStatusText(QStringLiteral("Generated %1 block suggestions.")
                    .arg(generated_blocks_.size()));
}

void BlockSliceViewModel::publishAll() {
  if (generated_blocks_.empty()) {
    setStatusText(QStringLiteral("Generate blocks first."));
    return;
  }

  publishing_ = true;
  emit publishingChanged();

  int published_count = 0;
  QString first_id;
  std::vector<PublishedBlock> published_blocks;
  published_blocks.reserve(generated_blocks_.size());
  for (const auto& generated : generated_blocks_) {
    BlockDraftBuilder builder(generated.id);
    builder.WithType(generated.type)
        .WithLanguage(generated.language)
        .WithDescription(generated.description)
        .WithTemplate(Template(generated.templ))
        .WithDefaults(generated.defaults);

    for (const auto& tag : generated.tags) {
      builder.WithTag(tag);
    }

    auto result =
        session_->engine().PublishBlock(builder.build(), Engine::VersionBump::Minor);
    if (result.HasError()) {
      publishing_ = false;
      emit publishingChanged();
      setStatusText(QString("Error publishing %1: %2")
                        .arg(QString::fromStdString(generated.id),
                             QString::fromStdString(result.error().message)));
      return;
    }

    ++published_count;
    published_blocks.push_back(result.value());
    if (first_id.isEmpty()) {
      first_id = QString::fromStdString(generated.id);
    }
  }

  QString composition_id;
  if (!published_blocks.empty()) {
    QString base_id = namespace_prefix_.trimmed().isEmpty()
                          ? QStringLiteral("sliced.composition")
                          : QStringLiteral("%1.composition")
                                .arg(namespace_prefix_.trimmed());
    QStringList existing_compositions;
    for (const auto& id : session_->engine().ListCompositions()) {
      existing_compositions.push_back(QString::fromStdString(id));
    }
    composition_id = UniqueCompositionId(base_id, existing_compositions);

    CompositionDraftBuilder builder(composition_id.toStdString());
    builder.WithDescription(QStringLiteral(
                                "Auto-generated composition from sliced prompt.")
                                .toStdString());
    for (const auto& block : published_blocks) {
      builder.AddBlockRef(block);
    }

    auto composition_result =
        session_->engine().PublishComposition(builder.build(),
                                             Engine::VersionBump::Minor);
    if (composition_result.HasError()) {
      publishing_ = false;
      emit publishingChanged();
      setStatusText(QString("Blocks published, but composition failed: %1")
                        .arg(QString::fromStdString(
                            composition_result.error().message)));
      return;
    }

    CompositionsViewModel::instance()->reload();
    CompositionsViewModel::instance()->selectComposition(composition_id);
  }

  publishing_ = false;
  emit publishingChanged();

  blocks_->reload();
  if (!first_id.isEmpty()) {
    blocks_->selectBlock(first_id);
  }
  setStatusText(
      composition_id.isEmpty()
          ? QStringLiteral("Published %1 blocks.").arg(published_count)
          : QStringLiteral("Published %1 blocks and composition %2.")
                .arg(published_count)
                .arg(composition_id));
  emit published();
  closeDialog();
}

void BlockSliceViewModel::resetForm() {
  source_prompt_text_.clear();
  namespace_prefix_.clear();
  language_ = QStringLiteral("en");
  generated_blocks_.clear();
  emit formChanged();
  emit generatedChanged();
}

void BlockSliceViewModel::setStatusText(QString value) {
  if (status_text_ == value) return;
  status_text_ = std::move(value);
  emit statusTextChanged();
}

QString BlockSliceViewModel::BuildGeneratedPreview() const {
  if (generated_blocks_.empty()) {
    return QStringLiteral("No generated blocks yet.");
  }

  QStringList lines;
  for (const auto& block : generated_blocks_) {
    lines << QStringLiteral("%1 [%2]")
                 .arg(QString::fromStdString(block.id), BlockTypeLabel(block.type));
    if (!block.description.empty()) {
      lines << QStringLiteral("  %1")
                   .arg(QString::fromStdString(block.description));
    }
    if (!block.templ.empty()) {
      lines << QStringLiteral("  ---");
      lines << QString::fromStdString(block.templ);
    }
    lines << QString();
  }
  return lines.join(QStringLiteral("\n")).trimmed();
}

QString BlockSliceViewModel::BuildCompositionPreviewId() const {
  if (generated_blocks_.empty()) {
    return QString();
  }

  QString base_id = namespace_prefix_.trimmed().isEmpty()
                        ? QStringLiteral("sliced.composition")
                        : QStringLiteral("%1.composition")
                              .arg(namespace_prefix_.trimmed());
  QStringList existing_compositions;
  for (const auto& id : session_->engine().ListCompositions()) {
    existing_compositions.push_back(QString::fromStdString(id));
  }
  return UniqueCompositionId(base_id, existing_compositions);
}

QString BlockSliceViewModel::BuildCompositionPreviewText() const {
  if (generated_blocks_.empty()) {
    return QStringLiteral("No composition will be created until blocks are generated.");
  }

  QStringList lines;
  lines << QStringLiteral("Composition: %1").arg(BuildCompositionPreviewId());
  lines << QStringLiteral("---");
  for (const auto& block : generated_blocks_) {
    lines << QStringLiteral("BlockRef %1")
                 .arg(QString::fromStdString(block.id));
  }
  return lines.join(QStringLiteral("\n"));
}

}  // namespace tf::gui
