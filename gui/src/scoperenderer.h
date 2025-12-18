#ifndef SCOPERENDERER_H
#define SCOPERENDERER_H

#include <vector>

#include <QColor>
#include <QImage>
#include <QList>
#include <QObject>
#include <QPainter>
#include <QString>

#include <osmium.h>

#include "renderargs.h"

class BaseRenderer : public QObject {
    Q_OBJECT
public:
    BaseRenderer(const QList<ChannelArgs>& channel_args, const GlobalArgs& global_args);

    int width() const;
    int height() const;
    void paint(QPainter&);

protected:
    struct PaintInfo {
        double x;
        double y;
        double w;
        double h;
        QPen wave_pen;
        QPen midline_pen;
        QString label;
        int program_num;
        int bank_num;

        void update_label(const ChannelArgs& args);
    };

    QColor m_border_color;
    double m_border_thickness;
    QColor m_background_color;
    int m_width;
    int m_height;
    int m_num_rows;
    int m_num_cols;
    bool m_debug_vis = false;

    std::vector<ChannelArgs> m_channel_args;
    std::vector<PaintInfo> m_paint_infos;

    virtual const std::vector<float>& get_left_wave(int index) const = 0;
    virtual const std::vector<float>& get_right_wave(int index) const = 0;

    void paint_borders(QPainter& painter);
    void paint_subframe(QPainter& painter, int index);
    void paint_wave(QPainter& painter,
                    const std::vector<float>& wave,
                    double w,
                    double h,
                    double mid_y);
};

class ScopeRenderer : public BaseRenderer {
public:
    ScopeRenderer(const QString& filename,
                  const QString& soundfont,
                  const QList<ChannelArgs>& channel_args,
                  const GlobalArgs& global_args);
    ScopeRenderer(const ScopeRenderer&) = delete;
    ScopeRenderer& operator=(const ScopeRenderer&) = delete;

    QImage paint_next_frame();
    bool has_frames_remaining() const;
    double get_progress();

protected:
    osmium::EventTracker m_event_tracker;
    std::vector<osmium::Scope> m_scopes;

    const std::vector<float>& get_left_wave(int index) const override;
    const std::vector<float>& get_right_wave(int index) const override;
};

class PreviewRenderer : public BaseRenderer {
public:
    PreviewRenderer(const QList<ChannelArgs>& channel_args,
                    const GlobalArgs& global_args);

protected:
    const std::vector<float>& get_left_wave(int index) const override;
    const std::vector<float>& get_right_wave(int index) const override;

private:
    std::vector<float> m_wave_data;
};

#endif // SCOPERENDERER_H
