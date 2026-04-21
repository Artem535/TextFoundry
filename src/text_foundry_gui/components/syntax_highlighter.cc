#include "components/syntax_highlighter.h"

#include <memory>

#include <QQuickTextDocument>
#include <QTextDocument>
#include <QVariant>

#ifdef TEXTFOUNDRY_HAS_KSYNTAXHIGHLIGHTING
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#endif

namespace tf::gui {

#ifdef TEXTFOUNDRY_HAS_KSYNTAXHIGHLIGHTING
class SyntaxHighlighter::Impl {
 public:
  KSyntaxHighlighting::Repository repository;
  std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> highlighter;
};
#endif

SyntaxHighlighter::SyntaxHighlighter(QObject* parent) : QObject(parent) {
#ifdef TEXTFOUNDRY_HAS_KSYNTAXHIGHLIGHTING
  impl_ = new Impl();
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

void SyntaxHighlighter::rebuildHighlighter() {
#ifdef TEXTFOUNDRY_HAS_KSYNTAXHIGHLIGHTING
  impl_->highlighter.reset();

  if (auto* document = resolveDocument(text_edit_)) {
    impl_->highlighter =
        std::make_unique<KSyntaxHighlighting::SyntaxHighlighter>(document);
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

  impl_->highlighter->setDefinition(
      impl_->repository.definitionForName(definition_));
  impl_->highlighter->setTheme(impl_->repository.defaultTheme(
      dark_theme_ ? KSyntaxHighlighting::Repository::DarkTheme
                  : KSyntaxHighlighting::Repository::LightTheme));
#endif
}

}  // namespace tf::gui
