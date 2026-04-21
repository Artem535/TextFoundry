#include "components/syntax_highlighter.h"

#include <QCoreApplication>
#include <QDir>
#include <QColor>
#include <QLoggingCategory>
#include <QPointer>
#include <QQuickTextDocument>
#include <QTextDocument>
#include <QVariant>

#ifdef TEXTFOUNDRY_HAS_KSYNTAXHIGHLIGHTING
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KSyntaxHighlighting/Theme>
#endif

namespace tf::gui {

Q_LOGGING_CATEGORY(kSyntaxHighlighterLog, "textfoundry.gui.syntaxhighlighter")

#ifdef TEXTFOUNDRY_HAS_KSYNTAXHIGHLIGHTING
class SyntaxHighlighter::Impl {
 public:
  KSyntaxHighlighting::Repository repository;
  QPointer<KSyntaxHighlighting::SyntaxHighlighter> highlighter;
};

static void AddRepositorySearchPaths(KSyntaxHighlighting::Repository& repository) {
  const QString app_dir = QCoreApplication::applicationDirPath();
  const QStringList search_paths = {
      QDir(app_dir).absoluteFilePath(QStringLiteral("../share/org.kde.syntax-highlighting")),
      QDir(app_dir).absoluteFilePath(QStringLiteral("../../share/org.kde.syntax-highlighting")),
#ifdef TEXTFOUNDRY_KSYNTAX_DATA_DIR
      QStringLiteral(TEXTFOUNDRY_KSYNTAX_DATA_DIR),
#endif
  };

  bool any_added = false;
  for (const QString& path : search_paths) {
    const QDir dir(path);
    if (!dir.exists()) {
      qCDebug(kSyntaxHighlighterLog) << "Skipping non-existent search path:" << path;
      continue;
    }

    if (dir.exists(QStringLiteral("syntax")) || dir.exists(QStringLiteral("themes"))) {
      qCInfo(kSyntaxHighlighterLog) << "Adding KSyntaxHighlighting search path:" << dir.absolutePath();
      repository.addCustomSearchPath(dir.absolutePath());
      any_added = true;
    }
  }

  if (!any_added) {
    qCWarning(kSyntaxHighlighterLog) << "No KSyntaxHighlighting data directories found!";
  }
}

static QColor ThemeColor(QRgb rgb) {
  if (rgb == 0) {
    return {};
  }
  return QColor::fromRgba(rgb);
}
#endif

SyntaxHighlighter::SyntaxHighlighter(QObject* parent) : QObject(parent) {
#ifdef TEXTFOUNDRY_HAS_KSYNTAXHIGHLIGHTING
  impl_ = new Impl();
  AddRepositorySearchPaths(impl_->repository);
#endif
}

SyntaxHighlighter::~SyntaxHighlighter() {
#ifdef TEXTFOUNDRY_HAS_KSYNTAXHIGHLIGHTING
  delete impl_;
#endif
}

QObject* SyntaxHighlighter::textEdit() const { return text_edit_; }

void SyntaxHighlighter::setTextEdit(QObject* value) {
  if (text_edit_ == value) {
    return;
  }
  text_edit_ = value;
  emit textEditChanged();
  rebuildHighlighter();
}

QString SyntaxHighlighter::definition() const { return definition_; }

void SyntaxHighlighter::setDefinition(const QString& value) {
  if (definition_ == value) {
    return;
  }
  definition_ = value;
  emit definitionChanged();
  applySettings();
}

bool SyntaxHighlighter::darkTheme() const { return dark_theme_; }

void SyntaxHighlighter::setDarkTheme(bool value) {
  if (dark_theme_ == value) {
    return;
  }
  dark_theme_ = value;
  emit darkThemeChanged();
  applySettings();
}

QColor SyntaxHighlighter::textColor() const { return text_color_; }

QColor SyntaxHighlighter::selectedTextColor() const { return selected_text_color_; }

QColor SyntaxHighlighter::selectionColor() const { return selection_color_; }

void SyntaxHighlighter::rebuildHighlighter() {
#ifdef TEXTFOUNDRY_HAS_KSYNTAXHIGHLIGHTING
  if (impl_->highlighter != nullptr) {
    impl_->highlighter->deleteLater();
    impl_->highlighter = nullptr;
  }

  if (auto* document = resolveDocument(text_edit_)) {
    impl_->highlighter = new KSyntaxHighlighting::SyntaxHighlighter(document);
    applySettings();
  }
#endif
}

QTextDocument* SyntaxHighlighter::resolveDocument(QObject* target) const {
  if (target == nullptr) {
    return nullptr;
  }

  if (auto* quick_document = qobject_cast<QQuickTextDocument*>(target)) {
    return quick_document->textDocument();
  }

  const QVariant text_document = target->property("textDocument");
  if (auto* quick_document =
          qobject_cast<QQuickTextDocument*>(text_document.value<QObject*>())) {
    return quick_document->textDocument();
  }

  return qobject_cast<QTextDocument*>(target);
}

void SyntaxHighlighter::applySettings() {
#ifdef TEXTFOUNDRY_HAS_KSYNTAXHIGHLIGHTING
  if (!impl_->highlighter) {
    return;
  }

  const auto definition = impl_->repository.definitionForName(definition_);
  const auto theme = impl_->repository.defaultTheme(
      dark_theme_ ? KSyntaxHighlighting::Repository::DarkTheme
                  : KSyntaxHighlighting::Repository::LightTheme);

  const auto* document = resolveDocument(text_edit_);
  qCInfo(kSyntaxHighlighterLog).noquote()
      << "Applying syntax highlighting:"
      << "requestedDefinition=" << definition_
      << "definitionValid=" << definition.isValid()
      << "definitionName=" << definition.name()
      << "definitionPath=" << definition.filePath()
      << "themeValid=" << theme.isValid()
      << "themeName=" << theme.name()
      << "documentChars=" << (document ? document->characterCount() : 0);

  impl_->highlighter->setDefinition(definition);
  impl_->highlighter->setTheme(theme);

  if (!theme.isValid()) {
    return;
  }

  QColor normal_text = ThemeColor(theme.textColor(KSyntaxHighlighting::Theme::Normal));
  if (!normal_text.isValid()) {
    normal_text = dark_theme_ ? QColor(0xE0, 0xE0, 0xE0) : QColor(0x20, 0x20, 0x20);
    qCWarning(kSyntaxHighlighterLog) << "Theme returned invalid normal text color, using fallback";
  }
  const QColor selected_text =
      ThemeColor(theme.selectedTextColor(KSyntaxHighlighting::Theme::Normal));
  const QColor selection =
      ThemeColor(theme.editorColor(KSyntaxHighlighting::Theme::TextSelection));

  const QColor resolved_selected_text =
      selected_text.isValid() ? selected_text : normal_text;

  bool colors_changed = false;
  if (text_color_ != normal_text) {
    text_color_ = normal_text;
    colors_changed = true;
  }
  if (selected_text_color_ != resolved_selected_text) {
    selected_text_color_ = resolved_selected_text;
    colors_changed = true;
  }
  if (selection_color_ != selection) {
    selection_color_ = selection;
    colors_changed = true;
  }
  if (colors_changed) {
    emit themeColorsChanged();
  }

  if (text_edit_ == nullptr) {
    qCWarning(kSyntaxHighlighterLog) << "Syntax highlighter has no target textEdit";
    return;
  }
#endif
}

}  // namespace tf::gui
