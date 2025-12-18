#include "previewer.h"

#include <QPaintEvent>

namespace controls {

Previewer::Previewer(QWidget* parent) : QWidget{parent} {}

void Previewer::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);

    int render_width, render_height; // NOLINT
    if (m_pixmap) {
        render_width = m_pixmap->width();
        render_height = m_pixmap->height();
    } else if (m_renderer) {
        render_width = m_renderer->width();
        render_height = m_renderer->height();
    } else {
        painter.fillRect(rect(), Qt::black);
        return;
    }

    double render_ratio = static_cast<double>(render_width) / render_height;
    double widget_ratio = static_cast<double>(width()) / height();
    QRect final_rect;
    if (render_ratio < widget_ratio) {
        // widget is wider than the render view => pillarbox
        int vp_width = height() * render_ratio;
        int x = (width() - vp_width) / 2;
        final_rect.setRect(x, 0, vp_width, height());
    } else {
        // widget is narrower than the render view => letterbox
        int vp_height = width() / render_ratio;
        int y = (height() - vp_height) / 2;
        final_rect.setRect(0, y, width(), vp_height);
    }
    painter.setWindow(0, 0, render_width, render_height);
    painter.setViewport(final_rect);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform
                           | QPainter::TextAntialiasing);

    if (m_pixmap) {
        painter.drawPixmap(0.0, 0.0, *m_pixmap);
    } else {
        m_renderer->paint(painter);
    }
}

void Previewer::update_args(const GlobalArgs& global_args,
                            const QList<ChannelArgs>& chan_args) {
    m_renderer.emplace(chan_args, global_args);
    update();
}

void Previewer::set_pixmap(const QPixmap& pixmap) {
    m_pixmap.emplace(pixmap);
    update();
}

void Previewer::clear_pixmap() {
    m_pixmap.reset();
    update();
}

} // namespace controls
