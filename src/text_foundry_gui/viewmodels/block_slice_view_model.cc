#include "viewmodels/block_slice_view_model.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <algorithm>
#include <cctype>
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

QString SummarizeText(QString text, const int max_length = 96) {
  text.replace('\n', ' ');
  text = text.simplified();
  if (text.size() > max_length) {
    text = text.left(max_length - 3) + QStringLiteral("...");
  }
  return text;
}

QString SummarizeMultiline(QString text, const int max_length = 220) {
  text.replace('\r', '\n');
  text = text.simplified();
  if (text.size() > max_length) {
    text = text.left(max_length - 3) + QStringLiteral("...");
  }
  return text;
}

QString UniqueBlockId(QString base_id, std::unordered_set<std::string>& used_ids) {
  QString candidate = std::move(base_id);
  int suffix = 2;
  while (used_ids.contains(candidate.toStdString())) {
    candidate = base_id + QStringLiteral("_") + QString::number(suffix++);
  }
  used_ids.insert(candidate.toStdString());
  return candidate;
}

QString UniqueCompositionId(QString base_id, const QStringList& existing_ids) {
  QString candidate = std::move(base_id);
  int suffix = 2;
  while (existing_ids.contains(candidate)) {
    candidate = base_id + QStringLiteral("_") + QString::number(suffix++);
  }
  return candidate;
}

std::optional<Version> ParseVersionText(const QString& input) {
  const auto parts = input.trimmed().split('.');
  if (parts.size() != 2) {
    return std::nullopt;
  }

  bool major_ok = false;
  bool minor_ok = false;
  const int major = parts[0].toInt(&major_ok);
  const int minor = parts[1].toInt(&minor_ok);
  if (!major_ok || !minor_ok || major < 0 || major > 65535 || minor < 0 ||
      minor > 65535) {
    return std::nullopt;
  }

  return Version{static_cast<uint16_t>(major), static_cast<uint16_t>(minor)};
}

QString DefaultNamespaceForComposition(const QString& composition_id) {
  const int dot = composition_id.lastIndexOf('.');
  if (dot > 0) {
    return composition_id.left(dot);
  }
  return composition_id.trimmed();
}

std::optional<std::vector<GeneratedBlockData>> ParseTaggedSections(
    const QString& source_text, const QString& language,
    const QString& namespace_prefix, const std::vector<BlockId>& existing_ids,
    const std::vector<BlockId>& reusable_ids = {}) {
  static const QRegularExpression kSectionPattern(
      QStringLiteral("<([A-Za-z_][A-Za-z0-9_]*)>\\s*([\\s\\S]*?)\\s*</\\1>"));

  std::unordered_set<std::string> used_ids(existing_ids.begin(), existing_ids.end());
  for (const auto& id : reusable_ids) {
    used_ids.erase(id);
  }
  std::vector<GeneratedBlockData> blocks;
  auto it = kSectionPattern.globalMatch(source_text);
  while (it.hasNext()) {
    const auto match = it.next();
    const QString section_name = match.captured(1).trimmed();
    const QString section_body = match.captured(2).trimmed();
    if (section_name.isEmpty() || section_body.isEmpty()) continue;

    const QString base_name = Slugify(section_name);
    QString reusable_id;
    const QString preferred_reusable_id = namespace_prefix.trimmed().isEmpty()
                                              ? QString()
                                              : namespace_prefix.trimmed() +
                                                    QStringLiteral(".") + base_name;
    for (const auto& existing_id : reusable_ids) {
      const QString existing = QString::fromStdString(existing_id);
      if (!preferred_reusable_id.isEmpty() && existing == preferred_reusable_id) {
        reusable_id = existing;
        break;
      }
      if (existing == base_name || existing.endsWith(QStringLiteral(".") + base_name)) {
        reusable_id = existing;
      }
    }

    const QString base_id = !reusable_id.isEmpty()
                                ? reusable_id
                                : namespace_prefix.trimmed().isEmpty()
                                      ? base_name
                                      : namespace_prefix.trimmed() +
                                            QStringLiteral(".") + base_name;

    blocks.push_back(GeneratedBlockData{
        .id = (!reusable_id.isEmpty() ? reusable_id : UniqueBlockId(base_id, used_ids))
                  .toStdString(),
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

QString BlockSliceViewModel::revisionComment() const { return revision_comment_; }

QString BlockSliceViewModel::dialogTitle() const {
  return mode_ == Mode::UpdateComposition
             ? QStringLiteral("Rewrite Prompt Into Blocks")
             : QStringLiteral("Slice Prompt Into Blocks");
}

QString BlockSliceViewModel::publishButtonText() const {
  return mode_ == Mode::UpdateComposition ? QStringLiteral("Update Prompt")
                                          : QStringLiteral("Publish All");
}

bool BlockSliceViewModel::updateMode() const {
  return mode_ == Mode::UpdateComposition;
}

int BlockSliceViewModel::preserveStructurePercent() const {
  return preserve_structure_percent_;
}

QString BlockSliceViewModel::targetCompositionId() const {
  return target_composition_id_;
}

QString BlockSliceViewModel::targetCompositionVersion() const {
  return target_composition_version_;
}

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
  invalidateGeneratedBlocks();
  emit formChanged();
}

void BlockSliceViewModel::setNamespacePrefix(const QString& value) {
  if (namespace_prefix_ == value) return;
  namespace_prefix_ = value;
  invalidateGeneratedBlocks();
  emit formChanged();
}

void BlockSliceViewModel::setLanguage(const QString& value) {
  if (language_ == value) return;
  language_ = value;
  invalidateGeneratedBlocks();
  emit formChanged();
}

void BlockSliceViewModel::setRevisionComment(const QString& value) {
  if (revision_comment_ == value) return;
  revision_comment_ = value;
  emit formChanged();
}

void BlockSliceViewModel::setPreserveStructurePercent(int value) {
  value = std::clamp(value, 10, 100);
  if (preserve_structure_percent_ == value) return;
  preserve_structure_percent_ = value;
  invalidateGeneratedBlocks();
  emit formChanged();
}

void BlockSliceViewModel::openDialog() {
  resetForm();
  mode_ = Mode::CreateBatch;
  open_ = true;
  setStatusText(QStringLiteral(
      "Paste prompt text, generate block suggestions, then publish the batch."));
  emit formChanged();
  emit generatedChanged();
  emit openChanged();
}

void BlockSliceViewModel::openUpdateDialog(const QString& compositionId,
                                           const QString& versionText) {
  resetForm();
  mode_ = Mode::UpdateComposition;
  target_composition_id_ = compositionId.trimmed();
  target_composition_version_ = versionText.trimmed();
  namespace_prefix_ = DefaultNamespaceForComposition(target_composition_id_);

  QString error_message;
  if (const auto composition = loadTargetComposition(&error_message);
      composition.has_value()) {
    auto rendered = session_->engine().Render(
        composition->id(),
        target_composition_version_.trimmed().isEmpty()
            ? composition->version()
            : *ParseVersionText(target_composition_version_));
    if (!rendered.HasError()) {
      source_prompt_text_ = QString::fromStdString(rendered.value().text);
      setStatusText(QStringLiteral(
          "Loaded current composition prompt. Edit it, then slice to publish updated blocks and composition."));
    } else {
      setStatusText(QStringLiteral(
          "Loaded target composition. Paste or edit the updated prompt text, then slice it."));
    }
  } else {
    setStatusText(error_message.isEmpty()
                      ? QStringLiteral("Select a composition first.")
                      : error_message);
  }

  open_ = true;
  emit formChanged();
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

  const auto reusable_ids = reusableBlockIdsForCurrentOperation();
  const auto disallowed_ids = disallowedExistingBlockIds();
  if (auto parsed = ParseTaggedSections(source_prompt_text_, language_,
                                        namespace_prefix_, disallowed_ids,
                                        reusable_ids);
      parsed.has_value()) {
    if (const auto validation_error = validateGeneratedBlocks(*parsed);
        validation_error.has_value()) {
      generated_blocks_.clear();
      generating_ = false;
      emit generatingChanged();
      emit generatedChanged();
      setStatusText(*validation_error);
      return;
    }
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
      .existing_block_ids = disallowed_ids,
      .reusable_block_ids = reusable_ids,
      .reusable_block_summaries = reusableBlockSummariesForTarget(),
      .preserve_reuse_percent = preserve_structure_percent_,
      .preserve_order = updateMode(),
      .allow_id_collision = !reusable_ids.empty(),
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

  auto generated = std::move(result).value().blocks;
  if (const auto validation_error = validateGeneratedBlocks(generated);
      validation_error.has_value()) {
    generated_blocks_.clear();
    emit generatedChanged();
    setStatusText(*validation_error);
    return;
  }

  generated_blocks_ = std::move(generated);
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

  const auto reusable_ids = reusableBlockIdsForCurrentOperation();
  const std::unordered_set<std::string> reusable_id_set(reusable_ids.begin(),
                                                        reusable_ids.end());
  int published_count = 0;
  QString first_id;
  std::vector<PublishedBlock> published_blocks;
  published_blocks.reserve(generated_blocks_.size());
  for (const auto& generated : generated_blocks_) {
    BlockDraftBuilder builder(generated.id);
    builder.WithType(generated.type)
        .WithLanguage(generated.language)
        .WithDescription(generated.description)
        .WithRevisionComment(defaultRevisionComment().toStdString())
        .WithTemplate(Template(generated.templ))
        .WithDefaults(generated.defaults);

    for (const auto& tag : generated.tags) {
      builder.WithTag(tag);
    }

    auto result = reusable_id_set.contains(generated.id)
                      ? session_->engine().UpdateBlock(
                            builder.build(), Engine::VersionBump::Minor)
                      : session_->engine().PublishBlock(
                            builder.build(), Engine::VersionBump::Minor);
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
  if (!published_blocks.empty() && mode_ == Mode::UpdateComposition) {
    QString error_message;
    auto composition = loadTargetComposition(&error_message);
    if (!composition.has_value()) {
      publishing_ = false;
      emit publishingChanged();
      setStatusText(error_message.isEmpty()
                        ? QStringLiteral("Target composition is not available.")
                        : error_message);
      return;
    }

    composition_id = QString::fromStdString(composition->id());
    CompositionDraftBuilder builder(composition->id());
    builder.WithDescription(composition->description())
        .WithRevisionComment(defaultRevisionComment().toStdString())
        .WithProjectKey(composition->ProjectKey());
    if (composition->GetStyleProfile().has_value()) {
      builder.WithStyleProfile(*composition->GetStyleProfile());
    }
    for (const auto& block : published_blocks) {
      builder.AddBlockRef(block);
    }

    auto composition_result =
        session_->engine().UpdateComposition(builder.build(),
                                            Engine::VersionBump::Minor);
    if (composition_result.HasError()) {
      publishing_ = false;
      emit publishingChanged();
      setStatusText(QString("Blocks published, but composition update failed: %1")
                        .arg(QString::fromStdString(
                            composition_result.error().message)));
      return;
    }

    CompositionsViewModel::instance()->reload();
    CompositionsViewModel::instance()->selectComposition(composition_id);
  } else if (!published_blocks.empty()) {
    composition_id = reusableCompositionIdForCurrentOperation();
    std::optional<Composition> existing_composition;
    if (!composition_id.isEmpty()) {
      auto existing_result =
          session_->engine().LoadComposition(composition_id.toStdString());
      if (!existing_result.HasError()) {
        existing_composition = std::move(existing_result).value();
      }
    }

    if (composition_id.isEmpty()) {
      QString base_id = namespace_prefix_.trimmed().isEmpty()
                            ? QStringLiteral("sliced.composition")
                            : namespace_prefix_.trimmed() +
                                  QStringLiteral(".composition");
      QStringList existing_compositions;
      for (const auto& id : session_->engine().ListCompositions()) {
        existing_compositions.push_back(QString::fromStdString(id));
      }
      composition_id = UniqueCompositionId(base_id, existing_compositions);
    }

    CompositionDraftBuilder builder(composition_id.toStdString());
    builder.WithDescription(existing_composition.has_value()
                                ? existing_composition->description()
                                : QStringLiteral(
                                      "Auto-generated composition from sliced prompt.")
                                      .toStdString())
        .WithRevisionComment(defaultRevisionComment().toStdString());
    if (existing_composition.has_value()) {
      builder.WithProjectKey(existing_composition->ProjectKey());
      if (existing_composition->GetStyleProfile().has_value()) {
        builder.WithStyleProfile(*existing_composition->GetStyleProfile());
      }
    }
    for (const auto& block : published_blocks) {
      builder.AddBlockRef(block);
    }

    auto composition_result = existing_composition.has_value()
                                  ? session_->engine().UpdateComposition(
                                        builder.build(), Engine::VersionBump::Minor)
                                  : session_->engine().PublishComposition(
                                        builder.build(), Engine::VersionBump::Minor);
    if (composition_result.HasError()) {
      publishing_ = false;
      emit publishingChanged();
      setStatusText(QString("Blocks published, but composition save failed: %1")
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
          : mode_ == Mode::UpdateComposition
                ? QStringLiteral("Published %1 blocks and updated composition %2.")
                      .arg(published_count)
                      .arg(composition_id)
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
  revision_comment_.clear();
  preserve_structure_percent_ = 70;
  target_composition_id_.clear();
  target_composition_version_.clear();
  generated_blocks_.clear();
  emit formChanged();
  emit generatedChanged();
}

void BlockSliceViewModel::invalidateGeneratedBlocks() {
  if (generated_blocks_.empty()) {
    return;
  }
  generated_blocks_.clear();
  emit generatedChanged();
  setStatusText(QStringLiteral(
      "Input changed. Generate again to refresh the sliced blocks."));
}

void BlockSliceViewModel::setStatusText(QString value) {
  if (status_text_ == value) return;
  status_text_ = std::move(value);
  session_->publishStatus(status_text_);
  emit statusTextChanged();
}

QString BlockSliceViewModel::BuildGeneratedPreview() const {
  if (generated_blocks_.empty()) {
    return QStringLiteral("No generated blocks yet.");
  }

  QStringList lines;
  std::unordered_set<std::string> reusable_ids;
  if (updateMode()) {
    const auto ids = reusableBlockIdsForCurrentOperation();
    reusable_ids.insert(ids.begin(), ids.end());
  }

  for (const auto& block : generated_blocks_) {
    const QString block_id = QString::fromStdString(block.id);
    const bool reused_existing = reusable_ids.contains(block.id);

    lines << QStringLiteral("%1 %2 [%3]")
                 .arg(reused_existing ? QStringLiteral("UPDATE")
                                      : QStringLiteral("NEW"),
                      block_id, BlockTypeLabel(block.type));

    if (reused_existing) {
      const auto existing_result = session_->engine().LoadBlock(block.id);
      if (!existing_result.HasError()) {
        const auto& existing = existing_result.value();
        const QString before_description =
            QString::fromStdString(existing.description()).trimmed();
        const QString after_description =
            QString::fromStdString(block.description).trimmed();
        if (!before_description.isEmpty() || !after_description.isEmpty()) {
          lines << QStringLiteral("  before desc: %1")
                       .arg(before_description.isEmpty()
                                ? QStringLiteral("(empty)")
                                : SummarizeMultiline(before_description, 140));
          lines << QStringLiteral("  after desc:  %1")
                       .arg(after_description.isEmpty()
                                ? QStringLiteral("(empty)")
                                : SummarizeMultiline(after_description, 140));
        }

        const QString before_template =
            SummarizeMultiline(QString::fromStdString(existing.templ().Content()));
        const QString after_template =
            SummarizeMultiline(QString::fromStdString(block.templ));
        lines << QStringLiteral("  before: %1").arg(before_template);
        lines << QStringLiteral("  after:  %1").arg(after_template);
      }
    } else {
      if (!block.description.empty()) {
        lines << QStringLiteral("  desc: %1")
                     .arg(SummarizeMultiline(
                         QString::fromStdString(block.description), 140));
      }
      if (!block.templ.empty()) {
        lines << QStringLiteral("  body: %1")
                     .arg(SummarizeMultiline(QString::fromStdString(block.templ)));
      }
    }
    lines << QString();
  }
  return lines.join(QStringLiteral("\n")).trimmed();
}

QString BlockSliceViewModel::BuildCompositionPreviewId() const {
  if (generated_blocks_.empty()) {
    return QString();
  }

  const QString reusable_id = reusableCompositionIdForCurrentOperation();
  if (!reusable_id.isEmpty()) {
    return reusable_id;
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

std::optional<Composition> BlockSliceViewModel::loadTargetComposition(
    QString* error_message) const {
  if (target_composition_id_.trimmed().isEmpty()) {
    if (error_message != nullptr) {
      *error_message = QStringLiteral("Select a composition first.");
    }
    return std::nullopt;
  }

  std::optional<Version> version;
  if (!target_composition_version_.trimmed().isEmpty()) {
    version = ParseVersionText(target_composition_version_);
    if (!version.has_value()) {
      if (error_message != nullptr) {
        *error_message = QStringLiteral("Selected composition version is invalid.");
      }
      return std::nullopt;
    }
  }

  auto result = session_->engine().LoadComposition(
      target_composition_id_.trimmed().toStdString(), version);
  if (result.HasError()) {
    if (error_message != nullptr) {
      *error_message =
          QString("Error: %1").arg(QString::fromStdString(result.error().message));
    }
    return std::nullopt;
  }

  return std::move(result).value();
}

std::vector<BlockId> BlockSliceViewModel::reusableBlockIdsForTarget() const {
  if (!updateMode()) {
    return {};
  }

  QString error_message;
  auto composition = loadTargetComposition(&error_message);
  if (!composition.has_value()) {
    return {};
  }

  std::vector<BlockId> ids;
  for (const auto& fragment : composition->fragments()) {
    if (!fragment.IsBlockRef()) {
      continue;
    }
    ids.push_back(fragment.AsBlockRef().GetBlockId());
  }
  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  return ids;
}

std::vector<std::string> BlockSliceViewModel::reusableBlockSummariesForTarget() const {
  if (!updateMode()) {
    return {};
  }

  QString error_message;
  auto composition = loadTargetComposition(&error_message);
  if (!composition.has_value()) {
    return {};
  }

  std::vector<std::string> summaries;
  std::unordered_set<std::string> seen_ids;
  for (const auto& fragment : composition->fragments()) {
    if (!fragment.IsBlockRef()) {
      continue;
    }

    const auto& block_ref = fragment.AsBlockRef();
    const auto& block_id = block_ref.GetBlockId();
    if (!seen_ids.insert(block_id).second) {
      continue;
    }

    auto block_result = session_->engine().LoadBlock(block_id);
    if (block_result.HasError()) {
      summaries.push_back(block_id);
      continue;
    }

    const auto& block = block_result.value();
    QString summary = QString::fromStdString(block_id);
    const QString description = QString::fromStdString(block.description()).trimmed();
    const QString templ = QString::fromStdString(block.templ().Content()).trimmed();
    QString body = !description.isEmpty() ? description : SummarizeText(templ);
    if (!body.isEmpty()) {
      summary += QStringLiteral(": ") + body;
    }
    summaries.push_back(summary.toStdString());
  }

  return summaries;
}

std::vector<BlockId> BlockSliceViewModel::reusableBlockIdsForNamespace() const {
  const QString prefix = namespace_prefix_.trimmed();
  if (prefix.isEmpty()) {
    return {};
  }

  const QString namespaced_prefix = prefix + QStringLiteral(".");
  std::vector<BlockId> ids;
  for (const auto& id : session_->engine().ListBlocks()) {
    const QString qid = QString::fromStdString(id);
    if (qid.startsWith(namespaced_prefix)) {
      ids.push_back(id);
    }
  }
  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  return ids;
}

std::vector<BlockId> BlockSliceViewModel::reusableBlockIdsForCurrentOperation() const {
  if (updateMode()) {
    return reusableBlockIdsForTarget();
  }
  return reusableBlockIdsForNamespace();
}

std::vector<BlockId> BlockSliceViewModel::disallowedExistingBlockIds() const {
  auto ids = session_->engine().ListBlocks();
  const auto reusable_ids = reusableBlockIdsForCurrentOperation();
  if (reusable_ids.empty()) {
    return ids;
  }
  std::unordered_set<std::string> reusable_set(reusable_ids.begin(),
                                               reusable_ids.end());
  std::vector<BlockId> filtered;
  filtered.reserve(ids.size());
  for (const auto& id : ids) {
    if (!reusable_set.contains(id)) {
      filtered.push_back(id);
    }
  }
  return filtered;
}

std::optional<QString> BlockSliceViewModel::validateGeneratedBlocks(
    const std::vector<GeneratedBlockData>& blocks) const {
  std::unordered_set<std::string> seen_ids;
  for (const auto& block : blocks) {
    if (!seen_ids.insert(block.id).second) {
      return QStringLiteral("Generated block ids must be unique.");
    }
  }

  const auto disallowed_block_ids = disallowedExistingBlockIds();
  const std::unordered_set<std::string> disallowed_ids(disallowed_block_ids.begin(),
                                                       disallowed_block_ids.end());
  for (const auto& block : blocks) {
    if (disallowed_ids.contains(block.id)) {
      return QString("Generated block id already exists and cannot be reused: %1")
          .arg(QString::fromStdString(block.id));
    }
  }

  if (updateMode()) {
    const auto reusable_ids = reusableBlockIdsForTarget();
    if (!reusable_ids.empty()) {
      std::unordered_set<std::string> reusable_set(reusable_ids.begin(),
                                                   reusable_ids.end());
      int retained_count = 0;
      QStringList retained_ids;
      for (const auto& block : blocks) {
        if (reusable_set.contains(block.id)) {
          ++retained_count;
          retained_ids.push_back(QString::fromStdString(block.id));
        }
      }

      const int original_count = static_cast<int>(reusable_ids.size());
      const int min_retained_count =
          std::max(1, (original_count * preserve_structure_percent_ + 99) / 100);

      if (retained_count < min_retained_count) {
        return QStringLiteral(
                   "Rewrite is too aggressive: preserved %1 of %2 existing blocks, "
                   "but Preserve Structure requires at least %3.\n"
                   "Retained blocks: %4")
            .arg(retained_count)
            .arg(original_count)
            .arg(min_retained_count)
            .arg(retained_ids.isEmpty() ? QStringLiteral("(none)")
                                        : retained_ids.join(QStringLiteral(", ")));
      }

      const int generated_count = static_cast<int>(blocks.size());
      if (generated_count < min_retained_count) {
        return QStringLiteral(
                   "Rewrite is too aggressive: generated only %1 blocks from %2 existing blocks.\n"
                   "With Preserve Structure %3%%, expected at least %4 blocks.")
            .arg(generated_count)
            .arg(original_count)
            .arg(preserve_structure_percent_)
            .arg(min_retained_count);
      }
    }
  }

  return std::nullopt;
}

QString BlockSliceViewModel::defaultRevisionComment() const {
  if (!revision_comment_.trimmed().isEmpty()) {
    return revision_comment_.trimmed();
  }
  return updateMode()
             ? QStringLiteral("Updated via AI slicing.")
             : QStringLiteral("Generated from sliced prompt.");
}

QString BlockSliceViewModel::reusableCompositionIdForCurrentOperation() const {
  if (mode_ == Mode::UpdateComposition) {
    return target_composition_id_.trimmed();
  }

  const QString prefix = namespace_prefix_.trimmed();
  if (prefix.isEmpty()) {
    return QString();
  }

  const QString candidate = prefix + QStringLiteral(".composition");
  for (const auto& id : session_->engine().ListCompositions()) {
    if (QString::fromStdString(id) == candidate) {
      return candidate;
    }
  }
  return QString();
}

}  // namespace tf::gui
