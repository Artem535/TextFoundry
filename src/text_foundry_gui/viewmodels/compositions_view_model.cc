#include "viewmodels/compositions_view_model.h"

#include <QCoreApplication>
#include <algorithm>
#include <ranges>
#include <string>

#include "app/session_view_model.h"
#include "tf/block_ref.h"
#include "tf/block_type.hpp"
#include "tf/composition.h"
#include "tf/fragment.h"

namespace tf::gui {
namespace {

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

QString PreviewStaticText(std::string text) {
  std::ranges::replace(text, '\n', ' ');
  if (text.size() > 96) {
    text = text.substr(0, 93) + "...";
  }
  return QString("Text: \"%1\"").arg(QString::fromStdString(text));
}

QString PreviewFragment(const Fragment& fragment) {
  if (fragment.IsBlockRef()) {
    const auto& ref = fragment.AsBlockRef();
    QString preview = QString("Block %1").arg(QString::fromStdString(ref.GetBlockId()));
    if (ref.version().has_value()) {
      preview += QString("@%1")
                     .arg(QString::fromStdString(ref.version()->ToString()));
    }
    if (!ref.LocalParams().empty()) {
      preview += QString(" [%1 params]").arg(ref.LocalParams().size());
    }
    return preview;
  }

  if (fragment.IsStaticText()) {
    return PreviewStaticText(fragment.AsStaticText().text());
  }

  return QString("Separator: %1")
      .arg(QString::fromUtf8(SeparatorTypeToString(fragment.AsSeparator().type).data(),
                             static_cast<int>(SeparatorTypeToString(fragment.AsSeparator().type).size())));
}

QString BuildCompositionSnapshot(const Composition& composition) {
  QStringList lines;
  lines << QString("Id: %1").arg(QString::fromStdString(composition.id()));
  lines << QString("Version: %1")
               .arg(QString::fromStdString(composition.version().ToString()));
  lines << QString("State: %1")
               .arg(QString::fromUtf8(BlockStateToString(composition.state()).data(),
                                      static_cast<int>(BlockStateToString(composition.state()).size())));

  const QString description = QString::fromStdString(composition.description()).trimmed();
  lines << QString("Description: %1")
               .arg(description.isEmpty() ? QStringLiteral("No description")
                                          : description);

  const QString revision_comment =
      QString::fromStdString(composition.revision_comment()).trimmed();
  lines << QString("Revision Comment: %1")
               .arg(revision_comment.isEmpty() ? QStringLiteral("No revision comment")
                                               : revision_comment);
  lines << QString();
  lines << QStringLiteral("Fragments:");

  int index = 1;
  for (const auto& fragment : composition.fragments()) {
    lines << QString("%1. %2").arg(index++).arg(PreviewFragment(fragment));
  }
  if (composition.fragments().empty()) {
    lines << QStringLiteral("No fragments");
  }

  return lines.join('\n');
}

}  // namespace

CompositionsViewModel::CompositionsViewModel(SessionViewModel* session,
                                             QObject* parent)
    : QObject(parent), session_(session) {
  Q_ASSERT(session_ != nullptr);
  connect(session_, &SessionViewModel::engineReset, this,
          &CompositionsViewModel::reload);
  reload();
}

CompositionsViewModel* CompositionsViewModel::create(QQmlEngine* qmlEngine,
                                                     QJSEngine* jsEngine) {
  Q_UNUSED(qmlEngine);
  Q_UNUSED(jsEngine);
  return instance();
}

CompositionsViewModel* CompositionsViewModel::instance() {
  static auto* singleton = new CompositionsViewModel(
      SessionViewModel::instance(), QCoreApplication::instance());
  return singleton;
}

QStringList CompositionsViewModel::compositionIds() const { return composition_ids_; }

QStringList CompositionsViewModel::filteredCompositionIds() const {
  return filtered_composition_ids_;
}

QString CompositionsViewModel::searchText() const { return search_text_; }

QString CompositionsViewModel::selectedCompositionId() const {
  return selected_composition_id_;
}

QString CompositionsViewModel::selectedVersion() const { return selected_version_; }

QStringList CompositionsViewModel::selectedVersions() const {
  return selected_versions_;
}

QStringList CompositionsViewModel::selectedVersionOptions() const {
  return selected_version_options_;
}

QVariantList CompositionsViewModel::versionEntries() const {
  return version_entries_;
}

QString CompositionsViewModel::selectedState() const { return selected_state_; }

QString CompositionsViewModel::selectedDescription() const {
  return selected_description_;
}

QString CompositionsViewModel::selectedRevisionComment() const {
  return selected_revision_comment_;
}

QString CompositionsViewModel::selectedFragmentCount() const {
  return selected_fragment_count_;
}

QStringList CompositionsViewModel::selectedFragments() const {
  return selected_fragments_;
}

QString CompositionsViewModel::tone() const { return tone_; }

QString CompositionsViewModel::tense() const { return tense_; }

QString CompositionsViewModel::targetLanguage() const { return target_language_; }

QString CompositionsViewModel::person() const { return person_; }

QString CompositionsViewModel::rewriteStrength() const { return rewrite_strength_; }

QString CompositionsViewModel::audience() const { return audience_; }

QString CompositionsViewModel::locale() const { return locale_; }

QString CompositionsViewModel::terminologyRigidity() const {
  return terminology_rigidity_;
}

bool CompositionsViewModel::preserveFormatting() const {
  return preserve_formatting_;
}

bool CompositionsViewModel::preserveExamples() const {
  return preserve_examples_;
}

bool CompositionsViewModel::normalizing() const { return normalizing_; }

bool CompositionsViewModel::normalizationAvailable() const {
  return session_->engine().HasBlockNormalizer();
}

QString CompositionsViewModel::normalizationStatusText() const {
  return normalization_status_text_;
}

QString CompositionsViewModel::statusText() const { return status_text_; }

bool CompositionsViewModel::compareOpen() const { return compare_open_; }

QString CompositionsViewModel::compareLeftTitle() const {
  return compare_left_title_;
}

QString CompositionsViewModel::compareRightTitle() const {
  return compare_right_title_;
}

QString CompositionsViewModel::compareLeftText() const { return compare_left_text_; }

QString CompositionsViewModel::compareRightText() const {
  return compare_right_text_;
}

QString CompositionsViewModel::compareSummary() const { return compare_summary_; }

void CompositionsViewModel::setSelectedCompositionId(const QString& value) {
  if (selected_composition_id_ == value) return;
  selected_composition_id_ = value;
  emit selectedCompositionIdChanged();
  refreshDetails();
}

void CompositionsViewModel::setSearchText(const QString& value) {
  if (search_text_ == value) return;
  search_text_ = value;
  refreshFilteredCompositions();
}

void CompositionsViewModel::setTone(const QString& value) {
  if (tone_ == value) return;
  tone_ = value;
  emit normalizationChanged();
}

void CompositionsViewModel::setTense(const QString& value) {
  if (tense_ == value) return;
  tense_ = value;
  emit normalizationChanged();
}

void CompositionsViewModel::setTargetLanguage(const QString& value) {
  if (target_language_ == value) return;
  target_language_ = value;
  emit normalizationChanged();
}

void CompositionsViewModel::setPerson(const QString& value) {
  if (person_ == value) return;
  person_ = value;
  emit normalizationChanged();
}

void CompositionsViewModel::setRewriteStrength(const QString& value) {
  if (rewrite_strength_ == value) return;
  rewrite_strength_ = value;
  emit normalizationChanged();
}

void CompositionsViewModel::setAudience(const QString& value) {
  if (audience_ == value) return;
  audience_ = value;
  emit normalizationChanged();
}

void CompositionsViewModel::setLocale(const QString& value) {
  if (locale_ == value) return;
  locale_ = value;
  emit normalizationChanged();
}

void CompositionsViewModel::setTerminologyRigidity(const QString& value) {
  if (terminology_rigidity_ == value) return;
  terminology_rigidity_ = value;
  emit normalizationChanged();
}

void CompositionsViewModel::setPreserveFormatting(const bool value) {
  if (preserve_formatting_ == value) return;
  preserve_formatting_ = value;
  emit normalizationChanged();
}

void CompositionsViewModel::setPreserveExamples(const bool value) {
  if (preserve_examples_ == value) return;
  preserve_examples_ = value;
  emit normalizationChanged();
}

void CompositionsViewModel::reload() {
  syncCompositions();
  refreshDetails();
}

void CompositionsViewModel::selectComposition(const QString& value) {
  setSelectedCompositionId(value);
}

void CompositionsViewModel::selectCompositionVersion(const QString& value) {
  if (value.isEmpty() || selected_version_ == value) return;
  selected_version_ = value;
  emit detailsChanged();
  refreshDetails();
}

void CompositionsViewModel::deprecateSelected() {
  if (selected_composition_id_.isEmpty()) {
    setStatusText(QStringLiteral("Select a composition first."));
    return;
  }

  Version version;
  if (const auto parse_error = ParseVersionText(selected_version_, version);
      parse_error.has_value()) {
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(*parse_error)));
    return;
  }

  const auto error = session_->engine().DeprecateComposition(
      selected_composition_id_.toStdString(), version);
  if (error.is_error()) {
    setStatusText(
        QString("Error: %1").arg(QString::fromStdString(error.message)));
    return;
  }

  reload();
  setStatusText(QStringLiteral("Deprecated selected composition version."));
}

void CompositionsViewModel::deleteSelected() {
  if (selected_composition_id_.isEmpty()) {
    setStatusText(QStringLiteral("Select a composition first."));
    return;
  }

  const auto deleted_id = selected_composition_id_;
  const auto error =
      session_->engine().DeleteComposition(selected_composition_id_.toStdString());
  if (error.is_error()) {
    setStatusText(
        QString("Error: %1").arg(QString::fromStdString(error.message)));
    return;
  }

  reload();
  setStatusText(QStringLiteral("Deleted composition %1.").arg(deleted_id));
}

void CompositionsViewModel::normalizeSelected() {
  if (!session_->engine().HasBlockNormalizer()) {
    setStatusText(QStringLiteral("Configure AI settings first."));
    return;
  }
  if (selected_composition_id_.isEmpty()) {
    setStatusText(QStringLiteral("Select a composition first."));
    return;
  }

  const auto style = currentSemanticStyle();
  if (style.isEmpty()) {
    setStatusText(QStringLiteral("Specify at least one semantic style field."));
    return;
  }

  Version version;
  if (const auto parse_error = ParseVersionText(selected_version_, version);
      parse_error.has_value()) {
    setStatusText(QString("Error: %1").arg(QString::fromStdString(*parse_error)));
    return;
  }

  normalizing_ = true;
  emit normalizationChanged();

  auto result = session_->engine().NormalizeComposition(
      CompositionNormalizationRequest{
          .source_composition_id = selected_composition_id_.toStdString(),
          .source_version = version,
          .style = style,
      });

  normalizing_ = false;
  emit normalizationChanged();

  if (result.HasError()) {
    normalization_status_text_ =
        QString("Error: %1").arg(QString::fromStdString(result.error().message));
    emit normalizationChanged();
    setStatusText(QStringLiteral("Composition normalization failed."));
    return;
  }

  normalization_status_text_ =
      QStringLiteral("Created normalized composition %1.")
          .arg(QString::fromStdString(result.value().composition_id));
  emit normalizationChanged();
  reload();
  setSelectedCompositionId(QString::fromStdString(result.value().composition_id));
  setStatusText(QStringLiteral("Normalized composition created."));
}

void CompositionsViewModel::updateBlocksToLatest() {
  if (selected_composition_id_.isEmpty()) {
    setStatusText(QStringLiteral("Select a composition first."));
    return;
  }

  Version version;
  if (const auto parse_error = ParseVersionText(selected_version_, version);
      parse_error.has_value()) {
    setStatusText(QString("Error: %1").arg(QString::fromStdString(*parse_error)));
    return;
  }

  auto composition_result = session_->engine().LoadComposition(
      selected_composition_id_.toStdString(), version);
  if (composition_result.HasError()) {
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(composition_result.error().message)));
    return;
  }

  const auto& composition = composition_result.value();
  CompositionDraftBuilder builder(composition.id());
  builder.WithDescription(composition.description())
      .WithRevisionComment("Updated block references to latest block versions.");

  bool changed = false;
  for (const auto& fragment : composition.fragments()) {
    if (fragment.IsBlockRef()) {
      const auto& ref = fragment.AsBlockRef();
      auto latest_result =
          session_->engine().GetLatestBlockVersion(ref.GetBlockId());
      if (latest_result.HasError()) {
        setStatusText(QString("Error: %1")
                          .arg(QString::fromStdString(latest_result.error().message)));
        return;
      }

      const auto latest_version = latest_result.value();
      if (!ref.version().has_value() || ref.version().value() != latest_version) {
        changed = true;
      }

      builder.AddBlockRef(
          BlockRef(ref.GetBlockId(), latest_version, ref.LocalParams()));
      continue;
    }

    if (fragment.IsStaticText()) {
      builder.AddStaticText(fragment.AsStaticText().text());
      continue;
    }

    if (fragment.IsSeparator()) {
      builder.AddSeparator(fragment.AsSeparator().type);
    }
  }

  if (!changed) {
    setStatusText(QStringLiteral("All block references already use latest versions."));
    return;
  }

  auto update_result = session_->engine().UpdateComposition(
      builder.build(), Engine::VersionBump::Minor);
  if (update_result.HasError()) {
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(update_result.error().message)));
    return;
  }

  reload();
  setSelectedCompositionId(
      QString::fromStdString(update_result.value().id()));
  selected_version_ =
      QString::fromStdString(update_result.value().version().ToString());
  refreshDetails();
  setStatusText(QStringLiteral("Updated block references to latest versions."));
}

void CompositionsViewModel::openCompareWithLatest() {
  if (selected_composition_id_.isEmpty()) {
    setStatusText(QStringLiteral("Select a composition first."));
    return;
  }

  const int current_index = selected_versions_.indexOf(selected_version_);
  if (current_index <= 0 || selected_versions_.isEmpty()) {
    setStatusText(QStringLiteral("Select a non-latest version for comparison."));
    return;
  }

  Version left_version;
  if (const auto parse_error =
          ParseVersionText(selected_versions_.front(), left_version);
      parse_error.has_value()) {
    setStatusText(QString("Error: %1").arg(QString::fromStdString(*parse_error)));
    return;
  }

  Version right_version;
  if (const auto parse_error =
          ParseVersionText(selected_versions_[current_index], right_version);
      parse_error.has_value()) {
    setStatusText(QString("Error: %1").arg(QString::fromStdString(*parse_error)));
    return;
  }

  auto left_result = session_->engine().LoadComposition(
      selected_composition_id_.toStdString(), left_version);
  if (left_result.HasError()) {
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(left_result.error().message)));
    return;
  }

  auto right_result = session_->engine().LoadComposition(
      selected_composition_id_.toStdString(), right_version);
  if (right_result.HasError()) {
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(right_result.error().message)));
    return;
  }

  compare_left_title_ = QString("Latest %1").arg(selected_versions_.front());
  compare_right_title_ =
      QString("Selected %1").arg(selected_versions_[current_index]);
  compare_left_text_ = BuildCompositionSnapshot(left_result.value());
  compare_right_text_ = BuildCompositionSnapshot(right_result.value());
  compare_summary_ =
      QString("%1: %2 vs %3")
          .arg(selected_composition_id_, selected_versions_.front(),
               selected_versions_[current_index]);
  compare_open_ = true;
  emit compareChanged();
}

void CompositionsViewModel::closeCompare() {
  if (!compare_open_) return;
  compare_open_ = false;
  emit compareChanged();
}

void CompositionsViewModel::syncCompositions() {
  auto ids = session_->engine().ListCompositions();
  std::sort(ids.begin(), ids.end());

  QStringList updated;
  updated.reserve(static_cast<qsizetype>(ids.size()));
  for (const auto& id : ids) {
    updated.push_back(QString::fromStdString(id));
  }

  const bool changed = composition_ids_ != updated;
  composition_ids_ = std::move(updated);
  refreshFilteredCompositions();
  if (changed) {
    emit compositionsChanged();
  }

  if (!selected_composition_id_.isEmpty() &&
      filtered_composition_ids_.contains(selected_composition_id_)) {
    return;
  }

  const QString next =
      filtered_composition_ids_.isEmpty() ? QString() : filtered_composition_ids_.front();
  if (selected_composition_id_ != next) {
    selected_composition_id_ = next;
    emit selectedCompositionIdChanged();
  }
}

void CompositionsViewModel::refreshFilteredCompositions() {
  const QString needle = search_text_.trimmed();
  QStringList filtered;
  filtered.reserve(composition_ids_.size());

  for (const auto& id : composition_ids_) {
    if (needle.isEmpty() || id.contains(needle, Qt::CaseInsensitive)) {
      filtered.push_back(id);
    }
  }

  if (filtered_composition_ids_ == filtered) return;
  filtered_composition_ids_ = std::move(filtered);
  emit compositionsChanged();
}

void CompositionsViewModel::refreshDetails() {
  if (selected_composition_id_.isEmpty()) {
    selected_version_.clear();
    selected_versions_.clear();
    selected_version_options_.clear();
    version_entries_.clear();
    selected_state_.clear();
    selected_description_.clear();
    selected_revision_comment_.clear();
    selected_fragment_count_.clear();
    selected_fragments_.clear();
    setStatusText(QStringLiteral("No compositions found."));
    emit detailsChanged();
    return;
  }

  auto versions_result = session_->engine().ListCompositionVersions(
      selected_composition_id_.toStdString());
  if (versions_result.HasError()) {
    selected_version_.clear();
    selected_versions_.clear();
    selected_version_options_.clear();
    version_entries_.clear();
    selected_state_.clear();
    selected_description_.clear();
    selected_revision_comment_.clear();
    selected_fragment_count_.clear();
    selected_fragments_.clear();
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(versions_result.error().message)));
    emit detailsChanged();
    return;
  }

  selected_versions_.clear();
  selected_version_options_.clear();
  version_entries_.clear();
  const auto& versions = versions_result.value();
  for (int i = 0; i < static_cast<int>(versions.size()); ++i) {
    const auto& version = versions[static_cast<size_t>(i)];
    const QString version_text = QString::fromStdString(version.ToString());
    selected_versions_.push_back(version_text);

    auto composition_result = session_->engine().LoadComposition(
        selected_composition_id_.toStdString(), version);
    QString label = version_text;
    QStringList markers;
    if (i == 0) {
      markers.push_back(QStringLiteral("latest"));
    }
    if (!composition_result.HasError() &&
        composition_result.value().state() == BlockState::Deprecated) {
      markers.push_back(QStringLiteral("deprecated"));
    }
    if (!markers.isEmpty()) {
      label += QStringLiteral(" (%1)").arg(markers.join(QStringLiteral(", ")));
    }
    selected_version_options_.push_back(label);

    QVariantMap entry;
    entry.insert(QStringLiteral("version"), version_text);
    entry.insert(QStringLiteral("label"), label);
    entry.insert(QStringLiteral("comment"),
                 !composition_result.HasError()
                     ? QString::fromStdString(
                           composition_result.value().revision_comment())
                     : QString());
    entry.insert(QStringLiteral("state"),
                 !composition_result.HasError()
                     ? QString::fromUtf8(
                           BlockStateToString(composition_result.value().state())
                               .data(),
                           static_cast<int>(BlockStateToString(
                                                composition_result.value().state())
                                                .size()))
                     : QString());
    entry.insert(QStringLiteral("isLatest"), i == 0);
    entry.insert(QStringLiteral("isSelected"), version_text == selected_version_);
    version_entries_.push_back(entry);
  }
  if (selected_version_.isEmpty() ||
      !selected_versions_.contains(selected_version_)) {
    selected_version_ =
        selected_versions_.isEmpty() ? QString() : selected_versions_.front();
  }

  Version selected_version;
  if (const auto parse_error = ParseVersionText(selected_version_, selected_version);
      parse_error.has_value()) {
    selected_state_.clear();
    selected_description_.clear();
    selected_revision_comment_.clear();
    selected_fragment_count_.clear();
    selected_fragments_.clear();
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(*parse_error)));
    emit detailsChanged();
    return;
  }

  auto result = session_->engine().LoadComposition(
      selected_composition_id_.toStdString(), selected_version);
  if (result.HasError()) {
    selected_versions_.clear();
    selected_version_options_.clear();
    version_entries_.clear();
    selected_state_.clear();
    selected_description_.clear();
    selected_revision_comment_.clear();
    selected_fragment_count_.clear();
    selected_fragments_.clear();
    setStatusText(QString("Error: %1")
                      .arg(QString::fromStdString(result.error().message)));
    emit detailsChanged();
    return;
  }

  const auto& composition = result.value();
  selected_version_ = QString::fromStdString(composition.version().ToString());
  selected_state_ = QString::fromUtf8(BlockStateToString(composition.state()).data(),
                                      static_cast<int>(BlockStateToString(composition.state()).size()));
  selected_description_ = QString::fromStdString(composition.description());
  selected_revision_comment_ =
      QString::fromStdString(composition.revision_comment());
  selected_fragment_count_ = QString::number(composition.fragmentCount());
  selected_fragments_.clear();
  selected_fragments_.reserve(static_cast<qsizetype>(composition.fragments().size()));
  for (const auto& fragment : composition.fragments()) {
    selected_fragments_.push_back(PreviewFragment(fragment));
  }

  for (int i = 0; i < version_entries_.size(); ++i) {
    auto entry = version_entries_[i].toMap();
    entry[QStringLiteral("isSelected")] =
        entry.value(QStringLiteral("version")).toString() == selected_version_;
    version_entries_[i] = entry;
  }

  setStatusText(QStringLiteral("Composition details loaded."));
  emit detailsChanged();
}

void CompositionsViewModel::setStatusText(QString value) {
  if (status_text_ == value) return;
  status_text_ = std::move(value);
  emit statusTextChanged();
}

SemanticStyle CompositionsViewModel::currentSemanticStyle() const {
  SemanticStyle style;
  if (!tone_.trimmed().isEmpty()) style.tone = tone_.trimmed().toStdString();
  if (!tense_.trimmed().isEmpty()) style.tense = tense_.trimmed().toStdString();
  if (!target_language_.trimmed().isEmpty()) {
    style.targetLanguage = target_language_.trimmed().toStdString();
  }
  if (!person_.trimmed().isEmpty()) style.person = person_.trimmed().toStdString();
  if (!rewrite_strength_.trimmed().isEmpty()) {
    style.rewriteStrength = rewrite_strength_.trimmed().toStdString();
  }
  if (!audience_.trimmed().isEmpty()) {
    style.audience = audience_.trimmed().toStdString();
  }
  if (!locale_.trimmed().isEmpty()) {
    style.locale = locale_.trimmed().toStdString();
  }
  if (!terminology_rigidity_.trimmed().isEmpty()) {
    style.terminologyRigidity = terminology_rigidity_.trimmed().toStdString();
  }
  style.preserveFormatting = preserve_formatting_;
  style.preserveExamples = preserve_examples_;
  return style;
}

}  // namespace tf::gui
