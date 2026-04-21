#pragma once

#include <QObject>
#include <QColor>
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
  Q_PROPERTY(QColor textColor READ textColor NOTIFY themeColorsChanged)
  Q_PROPERTY(QColor selectedTextColor READ selectedTextColor NOTIFY themeColorsChanged)
  Q_PROPERTY(QColor selectionColor READ selectionColor NOTIFY themeColorsChanged)

 public:
  explicit SyntaxHighlighter(QObject* parent = nullptr);
  ~SyntaxHighlighter() override;

  QObject* textEdit() const;
  void setTextEdit(QObject* value);

  QString definition() const;
  void setDefinition(const QString& value);

  bool darkTheme() const;
  void setDarkTheme(bool value);

  QColor textColor() const;
  QColor selectedTextColor() const;
  QColor selectionColor() const;

 signals:
  void textEditChanged();
  void definitionChanged();
  void darkThemeChanged();
  void themeColorsChanged();

 private:
  void rebuildHighlighter();
  QTextDocument* resolveDocument(QObject* target) const;
  void applySettings();

  QObject* text_edit_ = nullptr;
  QString definition_ = QStringLiteral("Markdown");
  bool dark_theme_ = false;
  QColor text_color_;
  QColor selected_text_color_;
  QColor selection_color_;

#ifdef TEXTFOUNDRY_HAS_KSYNTAXHIGHLIGHTING
  class Impl;
  Impl* impl_ = nullptr;
#endif
};

}  // namespace tf::gui
