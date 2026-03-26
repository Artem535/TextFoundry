#include "viewmodels/compositions_view_model.h"

#include <QCoreApplication>
#include <algorithm>
#include <ranges>
#include <string>

#include "app/session_view_model.h"
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

QString CompositionsViewModel::selectedCompositionId() const {
  return selected_composition_id_;
}

QString CompositionsViewModel::selectedVersion() const { return selected_version_; }

QString CompositionsViewModel::selectedState() const { return selected_state_; }

QString CompositionsViewModel::selectedDescription() const {
  return selected_description_;
}

QString CompositionsViewModel::selectedFragmentCount() const {
  return selected_fragment_count_;
}

QStringList CompositionsViewModel::selectedFragments() const {
  return selected_fragments_;
}

QString CompositionsViewModel::statusText() const { return status_text_; }

void CompositionsViewModel::setSelectedCompositionId(const QString& value) {
  if (selected_composition_id_ == value) return;
  selected_composition_id_ = value;
  emit selectedCompositionIdChanged();
  refreshDetails();
}

void CompositionsViewModel::reload() {
  syncCompositions();
  refreshDetails();
}

void CompositionsViewModel::selectComposition(const QString& value) {
  setSelectedCompositionId(value);
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
  if (changed) {
    emit compositionsChanged();
  }

  if (!selected_composition_id_.isEmpty() &&
      composition_ids_.contains(selected_composition_id_)) {
    return;
  }

  const QString next =
      composition_ids_.isEmpty() ? QString() : composition_ids_.front();
  if (selected_composition_id_ != next) {
    selected_composition_id_ = next;
    emit selectedCompositionIdChanged();
  }
}

void CompositionsViewModel::refreshDetails() {
  if (selected_composition_id_.isEmpty()) {
    selected_version_.clear();
    selected_state_.clear();
    selected_description_.clear();
    selected_fragment_count_.clear();
    selected_fragments_.clear();
    setStatusText(QStringLiteral("No compositions found."));
    emit detailsChanged();
    return;
  }

  auto result =
      session_->engine().LoadComposition(selected_composition_id_.toStdString());
  if (result.HasError()) {
    selected_version_.clear();
    selected_state_.clear();
    selected_description_.clear();
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
  selected_fragment_count_ = QString::number(composition.fragmentCount());
  selected_fragments_.clear();
  selected_fragments_.reserve(static_cast<qsizetype>(composition.fragments().size()));
  for (const auto& fragment : composition.fragments()) {
    selected_fragments_.push_back(PreviewFragment(fragment));
  }

  setStatusText(QStringLiteral("Composition details loaded."));
  emit detailsChanged();
}

void CompositionsViewModel::setStatusText(QString value) {
  if (status_text_ == value) return;
  status_text_ = std::move(value);
  emit statusTextChanged();
}

}  // namespace tf::gui
