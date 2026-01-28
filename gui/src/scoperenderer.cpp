#include "scoperenderer.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <numeric>
#include <vector>

#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QPolygonF>
#include <QRegularExpression>
#include <QtConcurrentMap>

#include <osmium.h>

#include "instrumentnames.h"

BaseRenderer::BaseRenderer(const QList<ChannelArgs>& channel_args,
                           const GlobalArgs& global_args)
    : m_width(global_args.width),
      m_height(global_args.height),
      m_border_color(global_args.border_color),
      m_border_thickness(global_args.border_thickness),
      m_background_color(global_args.background_color),
      m_channel_args(channel_args.cbegin(), channel_args.cend()) {
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
        const auto& args = channel_args[i];
        m_paint_infos.emplace_back(PaintInfo{
            .x = col * w,
            .y = row * h,
            .w = w,
            .h = h,
            .wave_pen = QPen(QColor(args.color), args.thickness),
            .midline_pen = QPen(QColor(args.midline_color), args.midline_thickness),
            .label = "",
            .program_num = 0,
            .bank_num = 0,
        });
        m_paint_infos.back().update_label(args);

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

void BaseRenderer::paint(QPainter& painter) {
    painter.fillRect(0, 0, m_width, m_height, m_background_color);
    const auto orig_transform = painter.worldTransform();
    for (int i = 0; i < m_paint_infos.size(); i++) {
        painter.translate(m_paint_infos[i].x, m_paint_infos[i].y);
        paint_subframe(painter, i);
        painter.setWorldTransform(orig_transform);
    }
    paint_borders(painter);
}

int BaseRenderer::width() const {
    return m_width;
}

int BaseRenderer::height() const {
    return m_height;
}

void BaseRenderer::paint_borders(QPainter& painter) {
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
}

void BaseRenderer::paint_subframe(QPainter& painter, int index) {
    const auto& p = m_paint_infos[index];
    const auto& args = m_channel_args[index];

    if (args.draw_v_midline) {
        painter.setPen(p.midline_pen);
        painter.drawLine(QLineF(p.w * 0.5, 0, p.w * 0.5, p.h)); // Vertical axis
    }

    if (args.draw_labels) {
        painter.setFont(args.label_font);
        painter.setPen(QColor(args.label_color));
        painter.drawText(
            QRectF(m_border_thickness * 0.5 + 3, m_border_thickness * 0.5 + 3, p.w, p.h),
            p.label);
    }

    if (args.is_stereo) {
        if (args.draw_h_midline) {
            painter.setPen(p.midline_pen);
            painter.drawLine(QLineF(0, p.h * 0.25, p.w, p.h * 0.25)); // H axis 1
            painter.drawLine(QLineF(0, p.h * 0.75, p.w, p.h * 0.75)); // H axis 2
        }

        painter.setPen(p.wave_pen);
        paint_wave(painter, get_left_wave(index), p.w, p.h * 0.5, p.h * 0.25);
        paint_wave(painter, get_right_wave(index), p.w, p.h * 0.5, p.h * 0.75);
    } else {
        if (args.draw_h_midline) {
            painter.setPen(p.midline_pen);
            painter.drawLine(0, p.h * 0.5, p.w, p.h * 0.5);
        }

        painter.setPen(p.wave_pen);
        paint_wave(painter, get_left_wave(index), p.w, p.h, p.h * 0.5);
    }
}

void BaseRenderer::paint_wave(
    QPainter& painter, const std::vector<float>& wave, double w, double h, double mid_y) {
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

    painter.drawPolyline(polygon);
}

// -- BaseRenderer::PaintInfo --

void BaseRenderer::PaintInfo::update_label(const ChannelArgs& args) {
    label = "";
    QRegularExpression re("%.");

    int last_match_end = 0;
    for (const auto& match : re.globalMatch(args.label_template)) {
        label += args.label_template.sliced(last_match_end,
                                            match.capturedStart() - last_match_end);
        last_match_end = match.capturedEnd();

        auto mstr = match.captured();
        switch (mstr[1].unicode()) {
        case 'i': // _i_nstrument name
            label += get_instrument_name(program_num, bank_num, args.channel_number == 9);
            break;
        case 'n': // channel _n_umber
            label += QString::number(args.channel_number + 1);
            break;
        case '%':
            label += '%';
            break;
        default:
            label += mstr;
        }
    }

    label += args.label_template.sliced(last_match_end);
}

// -- ScopeRenderer --

ScopeRenderer::ScopeRenderer(const QString& filename,
                             const QString& soundfont,
                             const QList<ChannelArgs>& channel_args,
                             const GlobalArgs& global_args)
    : BaseRenderer(channel_args, global_args),
      m_event_tracker(filename.toUtf8(), global_args.fps) {
    for (int i = 0; i < channel_args.size(); i++) {
        const auto& args = channel_args[i];
        auto scope = osmium::ScopeBuilder()
                         .amplification(args.amplification)
                         .avoid_drift_bias(args.avoid_drift_bias)
                         .display_window_ms(args.scope_width_ms)
                         .drift_window(args.drift_window_ms)
                         .frame_rate(global_args.fps)
                         .max_nudge_ms(args.max_nudge_ms)
                         .peak_bias(args.peak_bias)
                         .peak_threshold(args.peak_threshold)
                         .similarity_bias(args.similarity_bias)
                         .similarity_window_ms(args.similarity_window_ms)
                         .soundfonts({soundfont.toStdString()})
                         .stereo(args.is_stereo)
                         .trigger_threshold(args.trigger_threshold)
                         .build_from_midi_channel(filename.toUtf8(), args.channel_number);
        m_scopes.emplace_back(std::move(scope));
    }
}

QImage ScopeRenderer::paint_concurrent() {
    const auto render_hints = QPainter::Antialiasing | QPainter::TextAntialiasing;

    // Run each subimage in parallel
    std::vector<int> indices(m_scopes.size());
    std::iota(indices.begin(), indices.end(), 0);
    auto subframes = QtConcurrent::blockingMapped(indices, [this, render_hints](int idx) {
        auto& pinfo = m_paint_infos[idx];

        QImage subimg(std::ceil(pinfo.w), std::ceil(pinfo.h), QImage::Format_RGB32);
        subimg.fill(m_background_color);

        QPainter painter(&subimg);
        painter.setRenderHints(render_hints);
        paint_subframe(painter, idx);

        return subimg;
    });

    // Composite each subimage into the full image
    QImage full_frame(m_width, m_height, QImage::Format_RGB32);
    full_frame.fill(m_background_color);
    QPainter painter(&full_frame);
    painter.setRenderHints(render_hints);

    for (int i = 0; i < subframes.size(); i++) {
        const auto& pinfo = m_paint_infos[i];
        const auto& subimg = subframes[i];
        painter.drawImage(QPointF(pinfo.x, pinfo.y), subimg);
    }
    paint_borders(painter);

    return full_frame;
}

void ScopeRenderer::advance_frame() {
    // Update events (for tracking instrument changes)
    m_event_tracker.next_events();

    // Update labels for each channel
    for (const auto& event : m_event_tracker.get_events()) {
        if (event.event != osmium::Event::Program)
            continue;

        // I don't really like this double loop, but m_channel_args should be small.
        for (int i = 0; i < m_channel_args.size(); i++) {
            const auto& args = m_channel_args[i];

            if (!args.draw_labels || event.chan != args.channel_number)
                continue;

            auto& pinfo = m_paint_infos[i];
            pinfo.program_num = event.param;
            pinfo.update_label(args);
        }
    }

    // In parallel: update wave data
    QtConcurrent::blockingMap(m_scopes,
                              [](osmium::Scope& scope) { scope.next_wave_data(); });
}

bool ScopeRenderer::has_frames_remaining() const {
    return std::ranges::any_of(m_scopes, &osmium::Scope::is_playing);
}

double ScopeRenderer::get_progress() {
    double acc = 0.0;
    for (const auto& scope : m_scopes) {
        acc +=
            static_cast<double>(scope.get_current_progress()) / scope.get_total_samples();
    }
    return acc / m_scopes.size();
}

const std::vector<float>& ScopeRenderer::get_left_wave(int index) const {
    return m_scopes[index].get_left_samples();
}
const std::vector<float>& ScopeRenderer::get_right_wave(int index) const {
    return m_scopes[index].get_right_samples();
}

// -- PreviewRenderer --

PreviewRenderer::PreviewRenderer(const QList<ChannelArgs>& channel_args,
                                 const GlobalArgs& global_args)
    : BaseRenderer(channel_args, global_args) {
    m_wave_data.reserve(120);

    for (int i = 0; i < 120; i++) {
        m_wave_data.push_back(std::sin(i * 0.16 * std::numbers::pi) * 0.5);
    }
}

const std::vector<float>& PreviewRenderer::get_left_wave(int /*index*/) const {
    return m_wave_data;
}

const std::vector<float>& PreviewRenderer::get_right_wave(int /*index*/) const {
    return m_wave_data;
}
