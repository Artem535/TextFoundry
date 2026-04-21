#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqml.h>

class QTextDocument;

namespace tf::gui {

class SyntaxHighlighter : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(SyntaxHighlighter)
  Q_PROPERTY(QObject* textEdit READ textEdit WRITE setTextEdit NOTIFY textEditChanged)
  Q_PROPERTY(QString definition READ definition WRITE setDefinition NOTIFY definitionChanged)
  Q_PROPERTY(bool darkTheme READ darkTheme WRITE setDarkTheme NOTIFY darkThemeChanged)

 public:
  explicit SyntaxHighlighter(QObject* parent = nullptr);
  ~SyntaxHighlighter() override;

  QObject* textEdit() const;
  void setTextEdit(QObject* value);

  QString definition() const;
  void setDefinition(const QString& value);

  bool darkTheme() const;
  void setDarkTheme(bool value);

 signals:
  void textEditChanged();
  void definitionChanged();
  void darkThemeChanged();

 private:
  void rebuildHighlighter();
  QTextDocument* resolveDocument(QObject* target) const;
  void applySettings();

  QObject* text_edit_ = nullptr;
  QString definition_ = QStringLiteral("Markdown");
  bool dark_theme_ = false;

#ifdef TEXTFOUNDRY_HAS_KSYNTAXHIGHLIGHTING
  class Impl;
  Impl* impl_ = nullptr;
#endif
};

}  // namespace tf::gui
