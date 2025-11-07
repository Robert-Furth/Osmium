#include "scoperenderer.h"

#include <cmath>
#include <numeric>
#include <vector>

#include <QPainter>
#include <QPen>
#include <QPicture>
#include <QPointF>
#include <QPolygonF>
#include <QStandardItemModel>
#include <QtConcurrentMap>

#include <osmium.h>

#include "instrumentnames.h"

ScopeRenderer::ScopeRenderer(const QString& filename,
                             const QString& soundfont,
                             const QList<ChannelArgs>& channel_args,
                             const GlobalArgs& global_args)
    : m_width(global_args.width),
      m_height(global_args.height),
      m_debug_vis(global_args.debug_vis),
      m_background_color(global_args.background_color),
      m_border_color(global_args.border_color),
      m_border_thickness(global_args.border_thickness),
      m_event_tracker(filename.toUtf8(), global_args.fps) {
    int num_channels = channel_args.size();
    switch (global_args.order) {
    case ChannelOrder::ROW_MAJOR:
        m_num_cols = global_args.num_rows_or_cols;
        m_num_rows = (num_channels + m_num_cols - 1) / m_num_cols;
        break;
    case ChannelOrder::COLUMN_MAJOR:
        m_num_rows = global_args.num_rows_or_cols;
        m_num_cols = (num_channels + m_num_rows - 1) / m_num_rows;
        break;
    }

    // total_width / num_cols - (border_thickness * (num_cols - 1))
    double w = (static_cast<float>(m_width) / m_num_cols);
    double h = (static_cast<float>(m_height) / m_num_rows);

    int row = 0;
    int col = 0;

    for (int i = 0; i < num_channels; i++) {
        auto& args = channel_args[i];
        auto scope = osmium::ScopeBuilder()
                         .amplification(args.amplification)
                         .display_window_ms(args.scope_width_ms)
                         .frame_rate(global_args.fps)
                         .max_nudge_ms(args.max_nudge_ms)
                         .peak_bias(args.peak_bias)
                         .peak_threshold(args.peak_bias_factor)
                         .similarity_bias(args.similarity_bias)
                         .similarity_window_ms(args.similarity_window_ms)
                         .soundfonts({soundfont.toStdString()})
                         .stereo(args.is_stereo)
                         .trigger_threshold(args.trigger_threshold)
                         .build_from_midi_channel(filename.toUtf8(), i);

        m_scopes.emplace_back(std::move(scope));
        m_paint_infos.emplace_back(PaintInfo{
            .channel = i,
            .x = col * w,
            .y = row * h,
            .w = w,
            .h = h,
            .is_stereo = args.is_stereo,
            .color = args.color,
            .thickness = args.thickness,
            .label = get_instrument_name(0, 0, i == 9),
        });

        switch (global_args.order) {
        case ChannelOrder::ROW_MAJOR:
            if (++col >= m_num_cols) {
                col = 0;
                row++;
            }
            break;
        case ChannelOrder::COLUMN_MAJOR:
            if (++row >= m_num_rows) {
                row = 0;
                col++;
            }
        }
    }
}

double ScopeRenderer::get_progress() {
    double acc = 0.0;
    for (const auto& scope : m_scopes) {
        acc += static_cast<double>(scope.get_current_progress())
               / scope.get_total_samples();
    }
    return acc / m_scopes.size();
}

bool ScopeRenderer::has_frames_remaining() const {
    for (const auto& scope : m_scopes) {
        if (scope.is_playing())
            return true;
    }
    return false;
}

QImage ScopeRenderer::paint_frame() {
    std::vector<int> indices(m_scopes.size());
    std::iota(indices.begin(), indices.end(), 0);

    m_event_tracker.next_events();
    auto subimages = QtConcurrent::blockingMapped(indices, [this](int index) {
        m_scopes[index].next_wave_data();
        return paint_subimage(index);

        /*QImage subimage(picture.width(), picture.height(), QImage::Format_RGB32);
        subimage.fill(m_background_color);

        QPainter subimage_painter(&subimage);
        subimage_painter.setRenderHint(QPainter::Antialiasing);
        picture.play(&subimage_painter);
        return subimage;*/
    });

    QImage full_frame(m_width, m_height, QImage::Format_RGB32);
    full_frame.fill(m_background_color);
    QPainter painter(&full_frame);
    // painter.fillRect(0, 0, m_width, m_height, m_background_color);

    // Draw subimages
    for (int i = 0; i < subimages.size(); i++) {
        auto& meta = m_paint_infos[i];
        auto& subimage = subimages[i];
        painter.drawImage(QPointF(meta.x, meta.y), subimage);
    }

    // Draw borders
    if (m_border_thickness) {
        painter.setPen(QPen(m_border_color, m_border_thickness));
        for (int i = 1; i < m_num_rows; i++) {
            double y = i * static_cast<double>(m_height) / m_num_rows;
            painter.drawLine(QLineF(0, y, m_width, y));
        }
        for (int i = 1; i < m_num_cols; i++) {
            double x = i * static_cast<double>(m_width) / m_num_cols;
            painter.drawLine(QLineF(x, 0, x, m_height));
        }
    }

    return full_frame;
}

QImage ScopeRenderer::paint_subimage(int index) {
    auto& scope = m_scopes[index];
    auto& p = m_paint_infos[index];
    QImage img(std::ceil(p.w), std::ceil(p.h), QImage::Format_RGB32);

    if (m_debug_vis) {
        if (scope.get_no_nudges_found()) {
            img.fill(QColor(0x40, 0, 0));
        } else {
            QRgb rgb = qRgb(0, 0, 0);
            if (scope.m_flag1)
                rgb |= qRgb(0, 0x40, 0);
            if (scope.m_flag2)
                rgb |= qRgb(0, 0, 0x40);

            img.fill(QColor(rgb));
        }
    } else {
        img.fill(m_background_color);
    }
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(QPen(QColor(0x60, 0x60, 0x60), 1));
    painter.drawLine(p.w * 0.5, 0, p.w * 0.5, p.h); // Vertical axis

    if (p.is_stereo) {
        painter.drawLine(0, p.h * 0.25, p.w, p.h * 0.25); // H axis 1
        painter.drawLine(0, p.h * 0.75, p.w, p.h * 0.75); // H axis 2
        paint_wave(scope.get_left_samples(),
                   painter,
                   p.w,
                   p.h * 0.5,
                   p.h * 0.25,
                   p.thickness,
                   p.color);
        paint_wave(scope.get_right_samples(),
                   painter,
                   p.w,
                   p.h * 0.5,
                   p.h * 0.75,
                   p.thickness,
                   p.color);
    } else {
        painter.drawLine(0, p.h * 0.5, p.w, p.h * 0.5);
        paint_wave(scope.get_left_samples(),
                   painter,
                   p.w,
                   p.h,
                   p.h * 0.5,
                   p.thickness,
                   p.color);
    }

    for (const auto& event : m_event_tracker.get_events()) {
        if (event.chan == p.channel && event.event == MIDI_EVENT_PROGRAM) {
            p.label = get_instrument_name(event.param, 0, p.channel == 9);
        }
    }

    painter.setFont(QFont("Arial", 16));
    painter.setPen(QPen(QColorConstants::White, 2));
    painter.drawText(QRect(10, 0, p.w - 10, p.h), p.label);
    /*if (m_debug_vis) {
        // DEBUG: nudge window
        double nudge_px = p.w * scope.get_this_nudge_ms() / scope.get_window_size_ms();
        double max_nudge_px = p.w * scope.get_max_nudge_ms() / scope.get_window_size_ms();
        painter.setPen(QColor(0xff, 0xff, 0));
        painter.drawRect(p.w * 0.5 - nudge_px, 0, max_nudge_px, p.h);
    }*/

    return img;
}

void ScopeRenderer::paint_wave(const std::vector<float>& wave,
                               QPainter& painter,
                               double w,
                               double h,
                               double mid_y,
                               double thickness,
                               QRgb color) {
    double x_mult = w / (wave.size() - 1);
    double y_mult = h * -0.5; // negative so positive samples are higher
    double y_offs = mid_y;

    QPolygonF polygon;
    polygon.reserve(wave.size());
    for (int i = 0; i < wave.size(); i++) {
        double x = i * x_mult;
        double y = std::clamp(wave[i], -1.0f, 1.0f) * y_mult + y_offs;
        polygon << QPointF(x, y);
    }

    painter.setPen(QPen(QColor(color), thickness));
    painter.drawPolyline(polygon);
}
