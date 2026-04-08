#include "viewmodels/composition_block_rewrite_view_model.h"

#include <QCoreApplication>

#include <algorithm>
#include <sstream>
#include <unordered_map>

#include "app/session_view_model.h"
#include "tf/block.h"
#include "viewmodels/compositions_view_model.h"

namespace tf::gui {
namespace {

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

QString SummarizeText(QString text, const int max_length = 180) {
  text.replace('\n', ' ');
  text = text.simplified();
  if (text.size() > max_length) {
    text = text.left(max_length - 3) + QStringLiteral("...");
  }
  return text;
}

}  // namespace

CompositionBlockRewriteViewModel::CompositionBlockRewriteViewModel(
    SessionViewModel* session, QObject* parent)
    : QObject(parent), session_(session) {
  Q_ASSERT(session_ != nullptr);
  connect(session_, &SessionViewModel::engineReset, this,
          &CompositionBlockRewriteViewModel::availabilityChanged);
}

CompositionBlockRewriteViewModel* CompositionBlockRewriteViewModel::create(
    QQmlEngine* qmlEngine, QJSEngine* jsEngine) {
  Q_UNUSED(qmlEngine);
  Q_UNUSED(jsEngine);
  return instance();
}

CompositionBlockRewriteViewModel* CompositionBlockRewriteViewModel::instance() {
  static auto* singleton = new CompositionBlockRewriteViewModel(
      SessionViewModel::instance(), QCoreApplication::instance());
  return singleton;
}

bool CompositionBlockRewriteViewModel::open() const { return open_; }

QString CompositionBlockRewriteViewModel::targetCompositionId() const {
  return target_composition_id_;
}

QString CompositionBlockRewriteViewModel::targetCompositionVersion() const {
  return target_composition_version_;
}

QString CompositionBlockRewriteViewModel::instruction() const {
  return instruction_;
}

QString CompositionBlockRewriteViewModel::previewText() const {
  return preview_text_;
}

int CompositionBlockRewriteViewModel::patchCount() const {
  return preview_.has_value()
             ? static_cast<int>(preview_->patches.size())
             : 0;
}

QString CompositionBlockRewriteViewModel::statusText() const {
  return status_text_;
}

bool CompositionBlockRewriteViewModel::previewing() const { return previewing_; }

bool CompositionBlockRewriteViewModel::applying() const { return applying_; }

bool CompositionBlockRewriteViewModel::rewriteAvailable() const {
  return session_->engine().HasCompositionBlockRewriter();
}

void CompositionBlockRewriteViewModel::setInstruction(const QString& value) {
  if (instruction_ == value) return;
  instruction_ = value;
  preview_.reset();
  preview_text_.clear();
  emit formChanged();
  emit previewChanged();
}

void CompositionBlockRewriteViewModel::openDialog(const QString& compositionId,
                                                  const QString& versionText) {
  target_composition_id_ = compositionId.trimmed();
  target_composition_version_ = versionText.trimmed();
  instruction_.clear();
  preview_.reset();
  preview_text_.clear();
  open_ = true;
  setStatusText(QStringLiteral("Describe what should change without changing block structure."));
  emit openChanged();
  emit formChanged();
  emit previewChanged();
}

void CompositionBlockRewriteViewModel::closeDialog() {
  if (!open_) return;
  open_ = false;
  emit openChanged();
}

void CompositionBlockRewriteViewModel::preview() {
  if (!rewriteAvailable()) {
    setStatusText(QStringLiteral("Configure AI settings first."));
    return;
  }
  if (target_composition_id_.isEmpty()) {
    setStatusText(QStringLiteral("Select a composition first."));
    return;
  }
  if (instruction_.trimmed().isEmpty()) {
    setStatusText(QStringLiteral("Instruction is required."));
    return;
  }

  QString parse_error;
  const auto version = parsedVersion(&parse_error);
  if (!version.has_value()) {
    setStatusText(parse_error);
    return;
  }

  previewing_ = true;
  emit busyChanged();

  auto result = session_->engine().PreviewCompositionBlockRewrite(
      CompositionBlockRewriteRequest{
          .source_composition_id = target_composition_id_.toStdString(),
          .source_version = *version,
          .instruction = instruction_.trimmed().toStdString(),
      });

  previewing_ = false;
  emit busyChanged();

  if (result.HasError()) {
    preview_.reset();
    preview_text_.clear();
    emit previewChanged();
    setStatusText(
        QString("Error: %1").arg(QString::fromStdString(result.error().message)));
    return;
  }

  preview_ = result.value();
  preview_text_ = buildPreviewText(*preview_);
  emit previewChanged();
  setStatusText(QStringLiteral("Generated rewrite preview for %1 block(s).")
                    .arg(preview_->patches.size()));
}

void CompositionBlockRewriteViewModel::applyAll() {
  if (!preview_.has_value()) {
    setStatusText(QStringLiteral("Preview changes first."));
    return;
  }
  if (preview_->patches.empty()) {
    setStatusText(QStringLiteral("No block changes to apply."));
    return;
  }

  applying_ = true;
  emit busyChanged();

  auto result = session_->engine().ApplyCompositionBlockRewrite(*preview_);

  applying_ = false;
  emit busyChanged();

  if (result.HasError()) {
    setStatusText(
        QString("Error: %1").arg(QString::fromStdString(result.error().message)));
    return;
  }

  CompositionsViewModel::instance()->reload();
  CompositionsViewModel::instance()->selectComposition(
      QString::fromStdString(result.value().composition_id));
  CompositionsViewModel::instance()->selectCompositionVersion(
      QString("%1.%2")
          .arg(result.value().composition_version.major)
          .arg(result.value().composition_version.minor));

  setStatusText(QStringLiteral("Applied rewrite patches and published a new composition version."));
  emit applied();
  closeDialog();
}

void CompositionBlockRewriteViewModel::setStatusText(QString value) {
  if (status_text_ == value) return;
  status_text_ = std::move(value);
  session_->publishStatus(status_text_);
  emit statusTextChanged();
}

std::optional<Version> CompositionBlockRewriteViewModel::parsedVersion(
    QString* error_message) const {
  const auto parsed = ParseVersionText(target_composition_version_);
  if (!parsed.has_value() && error_message != nullptr) {
    *error_message = QStringLiteral("Invalid composition version.");
  }
  return parsed;
}

QString CompositionBlockRewriteViewModel::buildPreviewText(
    const CompositionBlockRewritePreview& preview) const {
  QStringList lines;
  auto composition_result = session_->engine().LoadComposition(
      preview.source_composition_id, preview.source_version);
  std::unordered_map<std::string, Block> source_blocks;
  if (!composition_result.HasError()) {
    for (const auto& fragment : composition_result.value().fragments()) {
      if (!fragment.IsBlockRef()) continue;
      const auto& ref = fragment.AsBlockRef();
      if (source_blocks.contains(ref.GetBlockId())) continue;
      auto block_result = ref.version().has_value()
                              ? session_->engine().LoadBlock(ref.GetBlockId(),
                                                             *ref.version())
                              : session_->engine().LoadBlock(ref.GetBlockId());
      if (!block_result.HasError()) {
        source_blocks.emplace(ref.GetBlockId(), block_result.value());
      }
    }
  }

  for (const auto& patch : preview.patches) {
    lines << QStringLiteral("PATCH %1").arg(QString::fromStdString(patch.block_id));
    if (!patch.rationale.empty()) {
      lines << QStringLiteral("reason: %1")
                   .arg(QString::fromStdString(patch.rationale));
    }
    const auto it = source_blocks.find(patch.block_id);
    if (it != source_blocks.end()) {
      const auto& source = it->second;
      if (patch.description.has_value()) {
        lines << QStringLiteral("before desc: %1")
                     .arg(SummarizeText(QString::fromStdString(source.description())));
        lines << QStringLiteral("after desc:  %1")
                     .arg(SummarizeText(QString::fromStdString(*patch.description)));
      }
      if (patch.templ.has_value()) {
        lines << QStringLiteral("before: %1")
                     .arg(SummarizeText(QString::fromStdString(
                         source.templ().Content())));
        lines << QStringLiteral("after:  %1")
                     .arg(SummarizeText(QString::fromStdString(*patch.templ)));
      }
    }
    if (patch.tags.has_value()) {
      QStringList tags;
      for (const auto& tag : *patch.tags) {
        tags.push_back(QString::fromStdString(tag));
      }
      lines << QStringLiteral("tags: %1")
                   .arg(tags.join(QStringLiteral(", ")));
    }
    lines << QStringLiteral("---");
  }
  return lines.join(QStringLiteral("\n"));
}

}  // namespace tf::gui
