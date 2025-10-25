#include "resizableqlabel.h"

ResizableQLabel::ResizableQLabel(QWidget* parent)
    : QLabel(parent) {
    setMinimumSize(1, 1);
}

int ResizableQLabel::heightForWidth(int width) const {
    if (m_pixmap.isNull())
        return height();
    double aspect_ratio = static_cast<double>(m_pixmap.height()) / m_pixmap.width();
    return width * aspect_ratio;
}

QSize ResizableQLabel::sizeHint() const {
    return QSize(width(), heightForWidth(width()));
}

QPixmap ResizableQLabel::scaledPixmap() const {
    return m_pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void ResizableQLabel::setPixmap(const QPixmap& pixmap) {
    m_pixmap = pixmap;
    QLabel::setPixmap(scaledPixmap());
}

void ResizableQLabel::resizeEvent(QResizeEvent* evt) {
    if (!m_pixmap.isNull()) {
        QLabel::setPixmap(scaledPixmap());
    }
}
