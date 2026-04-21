#include "components/svg_icon.h"

#include <QFile>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>

namespace tf::gui {

SvgIcon::SvgIcon(QQuickItem* parent) : QQuickPaintedItem(parent) {
  setAntialiasing(true);
  setOpaquePainting(false);
  updateGeometry();
}

SvgIcon::~SvgIcon() { delete renderer_; }

QUrl SvgIcon::source() const { return source_; }

void SvgIcon::setSource(const QUrl& value) {
  if (source_ == value) {
    return;
  }
  source_ = value;
  reloadRenderer();
  emit sourceChanged();
}

QColor SvgIcon::color() const { return color_; }

void SvgIcon::setColor(const QColor& value) {
  if (color_ == value) {
    return;
  }
  color_ = value;
  update();
  emit colorChanged();
}

int SvgIcon::iconWidth() const { return icon_width_; }

void SvgIcon::setIconWidth(int value) {
  if (icon_width_ == value) {
    return;
  }
  icon_width_ = value;
  updateGeometry();
  update();
  emit iconWidthChanged();
}

int SvgIcon::iconHeight() const { return icon_height_; }

void SvgIcon::setIconHeight(int value) {
  if (icon_height_ == value) {
    return;
  }
  icon_height_ = value;
  updateGeometry();
  update();
  emit iconHeightChanged();
}

void SvgIcon::paint(QPainter* painter) {
  if (painter == nullptr || renderer_ == nullptr || !renderer_->isValid()) {
    return;
  }

  const QSize target_size(qMax(1, icon_width_), qMax(1, icon_height_));
  QImage image(target_size, QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::transparent);

  {
    QPainter image_painter(&image);
    renderer_->render(&image_painter, QRectF(QPointF(0, 0), QSizeF(target_size)));
    image_painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    image_painter.fillRect(image.rect(), color_);
  }

  painter->drawImage(boundingRect().toRect(), image);
}

void SvgIcon::reloadRenderer() {
  delete renderer_;
  renderer_ = nullptr;

  const QString path = resolvedSourcePath();
  if (path.isEmpty()) {
    update();
    return;
  }

  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    update();
    return;
  }

  renderer_ = new QSvgRenderer(file.readAll(), this);
  update();
}

void SvgIcon::updateGeometry() {
  setImplicitWidth(icon_width_);
  setImplicitHeight(icon_height_);
  setWidth(icon_width_);
  setHeight(icon_height_);
}

QString SvgIcon::resolvedSourcePath() const {
  if (!source_.isValid()) {
    return {};
  }

  if (source_.isLocalFile()) {
    return source_.toLocalFile();
  }

  if (source_.scheme() == QStringLiteral("qrc")) {
    return QStringLiteral(":") + source_.path();
  }

  const QString source_string = source_.toString();
  if (source_string.startsWith(QLatin1Char(':'))) {
    return source_string;
  }

  return source_string;
}

}  // namespace tf::gui
