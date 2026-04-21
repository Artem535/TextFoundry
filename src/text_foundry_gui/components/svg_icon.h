#pragma once

#include <QColor>
#include <QQuickPaintedItem>
#include <QUrl>
#include <QtQml/qqml.h>

class QPainter;
class QSvgRenderer;

namespace tf::gui {

class SvgIcon : public QQuickPaintedItem {
  Q_OBJECT
  QML_NAMED_ELEMENT(SvgIcon)
  Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY(int iconWidth READ iconWidth WRITE setIconWidth NOTIFY iconWidthChanged)
  Q_PROPERTY(int iconHeight READ iconHeight WRITE setIconHeight NOTIFY iconHeightChanged)

 public:
  explicit SvgIcon(QQuickItem* parent = nullptr);
  ~SvgIcon() override;

  QUrl source() const;
  void setSource(const QUrl& value);

  QColor color() const;
  void setColor(const QColor& value);

  int iconWidth() const;
  void setIconWidth(int value);

  int iconHeight() const;
  void setIconHeight(int value);

  void paint(QPainter* painter) override;

 signals:
  void sourceChanged();
  void colorChanged();
  void iconWidthChanged();
  void iconHeightChanged();

 private:
  void reloadRenderer();
  void updateGeometry();
  QString resolvedSourcePath() const;

  QUrl source_;
  QColor color_ = Qt::white;
  int icon_width_ = 16;
  int icon_height_ = 16;
  QSvgRenderer* renderer_ = nullptr;
};

}  // namespace tf::gui
