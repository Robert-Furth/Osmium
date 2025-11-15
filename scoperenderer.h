#ifndef SCOPERENDERER_H
#define SCOPERENDERER_H

#include <vector>

#include <QColor>
#include <QList>
#include <QObject>
#include <QPainter>
#include <QPicture>
#include <QString>

#include <osmium.h>

#include "renderargs.h"

class ScopeRenderer : QObject {
    Q_OBJECT
public:
    ScopeRenderer(const QString& filename,
                  const QString& soundfont,
                  const QList<ChannelArgs>& channel_args,
                  const GlobalArgs& global_args);
    ScopeRenderer(const ScopeRenderer&) = delete;
    ScopeRenderer& operator=(const ScopeRenderer&) = delete;

    QImage paint_frame();
    bool has_frames_remaining() const;
    double get_progress();

public slots:

signals:
    void progress_updated(double progress);

private:
    struct PaintInfo {
        //int channel;
        double x;
        double y;
        double w;
        double h;
        QPen wave_pen;
        QPen midline_pen;
        /*bool is_stereo;
        QRgb wave_color;
        double wave_thickness;
        QRgb midline_color;
        double midline_thickness;
        bool draw_h_midline;
        bool draw_v_midline;*/
        QString label;
    };

    osmium::EventTracker m_event_tracker;
    std::vector<osmium::Scope> m_scopes;
    std::vector<PaintInfo> m_paint_infos;
    std::vector<ChannelArgs> m_channel_args;

    QColor m_border_color;
    double m_border_thickness;
    QColor m_background_color;
    int m_width;
    int m_height;
    int m_num_rows;
    int m_num_cols;
    bool m_debug_vis;

    QImage paint_subimage(int index);
    void paint_wave(const std::vector<float>& wave,
                    QPainter& painter,
                    double w,
                    double h,
                    double mid_y);
};

#endif // SCOPERENDERER_H
