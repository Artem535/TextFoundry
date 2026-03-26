#include "viewmodels/render_view_model.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QClipboard>

#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "app/session_view_model.h"
#include "tf/fragment.h"

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

std::optional<std::string> ParseRuntimeParams(const std::string& input,
                                              Params& out) {
  std::istringstream stream(input);
  std::string line;
  while (std::getline(stream, line)) {
    line = Trim(line);
    if (line.empty()) continue;

    size_t start = 0;
    while (start <= line.size()) {
      const auto comma = line.find(',', start);
      const std::string token = line.substr(
          start, comma == std::string::npos ? std::string::npos : comma - start);
      const std::string kv = Trim(token);
      if (!kv.empty()) {
        const auto eq = kv.find('=');
        if (eq == std::string::npos) {
          return "Invalid param format: '" + kv + "' (expected key=value)";
        }
        const std::string key = Trim(kv.substr(0, eq));
        const std::string value = Trim(kv.substr(eq + 1));
        if (key.empty()) {
          return "Parameter name cannot be empty";
        }
        out[key] = value;
      }

      if (comma == std::string::npos) break;
      start = comma + 1;
    }
  }
  return std::nullopt;
}

std::optional<std::string> ParseVersion(const std::string& input, Version& out) {
  const std::string text = Trim(input);
  if (text.empty()) return std::nullopt;

  const auto dot = text.find('.');
  if (dot == std::string::npos) {
    return "Invalid version format: expected major.minor";
  }

  const std::string major_str = Trim(text.substr(0, dot));
  const std::string minor_str = Trim(text.substr(dot + 1));
  if (major_str.empty() || minor_str.empty()) {
    return "Invalid version format: expected major.minor";
  }

  try {
    const auto major_num = std::stoi(major_str);
    const auto minor_num = std::stoi(minor_str);
    if (major_num < 0 || major_num > 65535 || minor_num < 0 ||
        minor_num > 65535) {
      return "Version numbers must be in range 0..65535";
    }
    out = Version{static_cast<uint16_t>(major_num),
                  static_cast<uint16_t>(minor_num)};
  } catch (const std::exception&) {
    return "Invalid version format: expected numeric major.minor";
  }

  return std::nullopt;
}

StructuralStyle EffectiveStyle(const Composition& composition) {
  if (composition.GetStyleProfile().has_value()) {
    return composition.GetStyleProfile()->structural;
  }
  return {};
}

std::string ApplyStructuralStyle(const std::vector<std::string>& fragment_texts,
                                 const StructuralStyle& style) {
  std::ostringstream result;
  if (style.preamble.has_value()) {
    result << *style.preamble;
  }

  for (size_t i = 0; i < fragment_texts.size(); ++i) {
    std::string text = fragment_texts[i];
    if (style.blockWrapper.has_value()) {
      std::string wrapped = *style.blockWrapper;
      constexpr std::string_view kContent = "{{content}}";
      if (const size_t pos = wrapped.find(kContent); pos != std::string::npos) {
        wrapped.replace(pos, kContent.size(), text);
      }
      text = std::move(wrapped);
    }

    result << text;
    if (i + 1 < fragment_texts.size() && style.delimiter.has_value()) {
      result << *style.delimiter;
    }
  }

  if (style.postamble.has_value()) {
    result << *style.postamble;
  }
  return result.str();
}

Result<std::string> BuildRawRender(Engine& engine, const Composition& composition) {
  std::vector<std::string> fragment_texts;
  fragment_texts.reserve(composition.fragments().size());

  for (const auto& fragment : composition.fragments()) {
    if (fragment.IsStaticText()) {
      fragment_texts.push_back(fragment.AsStaticText().text());
      continue;
    }
    if (fragment.IsSeparator()) {
      fragment_texts.push_back(fragment.AsSeparator().toString());
      continue;
    }

    const auto& block_ref = fragment.AsBlockRef();
    auto block_result = block_ref.version().has_value()
                            ? engine.LoadBlock(block_ref.GetBlockId(),
                                               *block_ref.version())
                            : engine.LoadBlock(block_ref.GetBlockId());
    if (block_result.HasError()) {
      return Result<std::string>(block_result.error());
    }
    fragment_texts.push_back(block_result.value().templ().Content());
  }

  return Result<std::string>(
      ApplyStructuralStyle(fragment_texts, EffectiveStyle(composition)));
}

}  // namespace

RenderViewModel::RenderViewModel(SessionViewModel* session, QObject* parent)
    : QObject(parent), session_(session) {
  Q_ASSERT(session_ != nullptr);
  connect(session_, &SessionViewModel::engineReset, this, &RenderViewModel::reload);
  reload();
}

RenderViewModel* RenderViewModel::create(QQmlEngine* qmlEngine,
                                         QJSEngine* jsEngine) {
  Q_UNUSED(qmlEngine);
  Q_UNUSED(jsEngine);
  return instance();
}

RenderViewModel* RenderViewModel::instance() {
  static auto* singleton = new RenderViewModel(SessionViewModel::instance(),
                                               QCoreApplication::instance());
  return singleton;
}

QStringList RenderViewModel::compositionIds() const { return composition_ids_; }

QString RenderViewModel::selectedCompositionId() const {
  return selected_composition_id_;
}

QString RenderViewModel::versionText() const { return version_text_; }

QString RenderViewModel::paramsText() const { return params_text_; }

QString RenderViewModel::outputText() const { return output_text_; }

QString RenderViewModel::rawOutputText() const { return raw_output_text_; }

QString RenderViewModel::previewMode() const { return preview_mode_; }

QStringList RenderViewModel::previewModes() const {
  return {QStringLiteral("Raw"), QStringLiteral("Rendered")};
}

QString RenderViewModel::displayedOutputText() const {
  if (output_text_ != QStringLiteral("Select a composition and click Render.")) {
    return output_text_;
  }
  return raw_output_text_;
}

QString RenderViewModel::statusText() const { return status_text_; }

void RenderViewModel::setSelectedCompositionId(const QString& value) {
  if (selected_composition_id_ == value) return;
  selected_composition_id_ = value;
  emit selectedCompositionIdChanged();
  refreshRawPreview();
}

void RenderViewModel::setVersionText(const QString& value) {
  if (version_text_ == value) return;
  version_text_ = value;
  emit versionTextChanged();
  refreshRawPreview();
}

void RenderViewModel::setParamsText(const QString& value) {
  if (params_text_ == value) return;
  params_text_ = value;
  emit paramsTextChanged();
}

void RenderViewModel::setPreviewMode(const QString& value) {
  if (preview_mode_ == value) return;
  preview_mode_ = value;
  emit previewModeChanged();
  emit displayedOutputTextChanged();
}

void RenderViewModel::reload() {
  syncCompositions();
  refreshRawPreview();
  setStatusText(QStringLiteral("Compositions list refreshed."));
}

void RenderViewModel::render() {
  if (selected_composition_id_.trimmed().isEmpty()) {
    setOutputText(QStringLiteral("Error: Composition ID is required."));
    setRawOutputText(QStringLiteral("Error: Composition ID is required."));
    setStatusText(QStringLiteral("Select a composition before rendering."));
    return;
  }

  RenderContext context;
  if (const auto parse_err =
          ParseRuntimeParams(params_text_.toStdString(), context.params);
      parse_err.has_value()) {
    setOutputText(QString("Error: %1").arg(QString::fromStdString(*parse_err)));
    setRawOutputText(QString("Error: %1").arg(QString::fromStdString(*parse_err)));
    setStatusText(QStringLiteral("Runtime params are invalid."));
    return;
  }

  Version version;
  if (const auto parse_err =
          ParseVersion(version_text_.toStdString(), version);
      parse_err.has_value()) {
    setOutputText(QString("Error: %1").arg(QString::fromStdString(*parse_err)));
    setRawOutputText(QString("Error: %1").arg(QString::fromStdString(*parse_err)));
    setStatusText(QStringLiteral("Version format is invalid."));
    return;
  }

  auto result = version_text_.trimmed().isEmpty()
                    ? session_->engine().Render(selected_composition_id_.toStdString(),
                                                context)
                    : session_->engine().Render(selected_composition_id_.toStdString(),
                                                version, context);
  if (result.HasError()) {
    setOutputText(
        QString("Error: %1").arg(QString::fromStdString(result.error().message)));
    setRawOutputText(
        QString("Error: %1").arg(QString::fromStdString(result.error().message)));
    setStatusText(QStringLiteral("Render failed."));
    return;
  }

  setOutputText(QString::fromStdString(result.value().text));
  auto raw_result = buildRawOutput();
  if (raw_result.HasError()) {
    setRawOutputText(
        QString("Error: %1").arg(QString::fromStdString(raw_result.error().message)));
    setStatusText(QStringLiteral("Rendered successfully, raw preview failed."));
    return;
  }
  setRawOutputText(raw_result.value());
  setStatusText(QStringLiteral("Rendered successfully."));
}

void RenderViewModel::copyRender() {
  if (output_text_.isEmpty() || output_text_.startsWith(QStringLiteral("Error:")) ||
      output_text_ == QStringLiteral("Select a composition and click Render.")) {
    setStatusText(QStringLiteral("Nothing to copy from render output."));
    return;
  }

  if (auto* clipboard = QGuiApplication::clipboard(); clipboard != nullptr) {
    clipboard->setText(output_text_);
    setStatusText(QStringLiteral("Copied render output to clipboard."));
    return;
  }

  setStatusText(QStringLiteral("Clipboard is not available."));
}

void RenderViewModel::copyRaw() {
  if (raw_output_text_.isEmpty() || raw_output_text_.startsWith(QStringLiteral("Error:")) ||
      raw_output_text_ == QStringLiteral("Select a composition and click Render.")) {
    setStatusText(QStringLiteral("Nothing to copy from raw output."));
    return;
  }

  if (auto* clipboard = QGuiApplication::clipboard(); clipboard != nullptr) {
    clipboard->setText(raw_output_text_);
    setStatusText(QStringLiteral("Copied raw composition render to clipboard."));
    return;
  }

  setStatusText(QStringLiteral("Clipboard is not available."));
}

void RenderViewModel::clear() {
  setParamsText(QString());
  setVersionText(QString());
  setOutputText(QStringLiteral("Select a composition and click Render."));
  refreshRawPreview();
  setStatusText(QStringLiteral("Render state cleared."));
}

void RenderViewModel::setOutputText(QString value) {
  if (output_text_ == value) return;
  output_text_ = std::move(value);
  emit outputTextChanged();
  emit displayedOutputTextChanged();
}

void RenderViewModel::setRawOutputText(QString value) {
  if (raw_output_text_ == value) return;
  raw_output_text_ = std::move(value);
  emit rawOutputTextChanged();
  emit displayedOutputTextChanged();
}

void RenderViewModel::setStatusText(QString value) {
  if (status_text_ == value) return;
  status_text_ = std::move(value);
  emit statusTextChanged();
}

Result<QString> RenderViewModel::buildRawOutput() const {
  if (selected_composition_id_.trimmed().isEmpty()) {
    return Result<QString>(QStringLiteral("Select a composition to preview raw output."));
  }

  Version version;
  if (const auto parse_err = ParseVersion(version_text_.toStdString(), version);
      parse_err.has_value()) {
    return Result<QString>(
        Error{ErrorCode::InvalidVersion, *parse_err});
  }

  auto composition_result =
      version_text_.trimmed().isEmpty()
          ? session_->engine().LoadComposition(selected_composition_id_.toStdString())
          : session_->engine().LoadComposition(selected_composition_id_.toStdString(),
                                               version);
  if (composition_result.HasError()) {
    return Result<QString>(composition_result.error());
  }

  auto raw_result = BuildRawRender(session_->engine(), composition_result.value());
  if (raw_result.HasError()) {
    return Result<QString>(raw_result.error());
  }

  return Result<QString>(QString::fromStdString(raw_result.value()));
}

void RenderViewModel::refreshRawPreview() {
  auto raw_result = buildRawOutput();
  if (raw_result.HasError()) {
    setRawOutputText(
        QString("Error: %1").arg(QString::fromStdString(raw_result.error().message)));
    return;
  }
  setRawOutputText(raw_result.value());
}

void RenderViewModel::syncCompositions() {
  auto ids = session_->engine().ListCompositions();
  std::sort(ids.begin(), ids.end());

  QStringList updated;
  updated.reserve(static_cast<qsizetype>(ids.size()));
  for (const auto& id : ids) {
    updated.push_back(QString::fromStdString(id));
  }

  const bool compositions_changed = composition_ids_ != updated;
  composition_ids_ = std::move(updated);
  if (compositions_changed) {
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

}  // namespace tf::gui
