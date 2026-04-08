#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqml.h>

#include <optional>

#include "tf/engine.h"

namespace tf::gui {

class SessionViewModel;

class CompositionBlockRewriteViewModel : public QObject {
  Q_OBJECT
  QML_SINGLETON
  QML_NAMED_ELEMENT(CompositionBlockRewriteVm)
  Q_PROPERTY(bool open READ open NOTIFY openChanged)
  Q_PROPERTY(QString targetCompositionId READ targetCompositionId NOTIFY formChanged)
  Q_PROPERTY(QString targetCompositionVersion READ targetCompositionVersion NOTIFY formChanged)
  Q_PROPERTY(QString instruction READ instruction WRITE setInstruction NOTIFY formChanged)
  Q_PROPERTY(QString previewText READ previewText NOTIFY previewChanged)
  Q_PROPERTY(int patchCount READ patchCount NOTIFY previewChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
  Q_PROPERTY(bool previewing READ previewing NOTIFY busyChanged)
  Q_PROPERTY(bool applying READ applying NOTIFY busyChanged)
  Q_PROPERTY(bool rewriteAvailable READ rewriteAvailable NOTIFY availabilityChanged)

 public:
  explicit CompositionBlockRewriteViewModel(SessionViewModel* session,
                                            QObject* parent = nullptr);
  static CompositionBlockRewriteViewModel* create(QQmlEngine* qmlEngine,
                                                  QJSEngine* jsEngine);
  static CompositionBlockRewriteViewModel* instance();

  bool open() const;
  QString targetCompositionId() const;
  QString targetCompositionVersion() const;
  QString instruction() const;
  QString previewText() const;
  int patchCount() const;
  QString statusText() const;
  bool previewing() const;
  bool applying() const;
  bool rewriteAvailable() const;

  void setInstruction(const QString& value);

  Q_INVOKABLE void openDialog(const QString& compositionId,
                              const QString& versionText);
  Q_INVOKABLE void closeDialog();
  Q_INVOKABLE void preview();
  Q_INVOKABLE void applyAll();

 signals:
  void openChanged();
  void formChanged();
  void previewChanged();
  void statusTextChanged();
  void busyChanged();
  void availabilityChanged();
  void applied();

 private:
  void setStatusText(QString value);
  std::optional<Version> parsedVersion(QString* error_message) const;
  QString buildPreviewText(const CompositionBlockRewritePreview& preview) const;

  SessionViewModel* session_;
  bool open_ = false;
  bool previewing_ = false;
  bool applying_ = false;
  QString target_composition_id_;
  QString target_composition_version_;
  QString instruction_;
  QString preview_text_;
  QString status_text_;
  std::optional<CompositionBlockRewritePreview> preview_;
};

}  // namespace tf::gui
