#ifndef SCOPERENDERER_H
#define SCOPERENDERER_H

#include <vector>

#include <QColor>
#include <QList>
#include <QObject>
#include <QPainter>
#include <QPicture>
#include <QString>

#include "osmium/osmium.h"

class ScopeRenderer : QObject {
    Q_OBJECT
public:
    enum class ChannelOrder { ROW_MAJOR, COLUMN_MAJOR };

    struct GlobalArgs {
        int width;
        int height;
        int num_rows_or_cols;
        ChannelOrder order;
        int fps;
        QRgb border_color;
        int border_thickness;
        bool debug_vis;
    };

    struct ChannelArgs {
        int scope_width_ms;
        int max_nudge_ms;
        double amplification;
        double trigger_threshold;
        bool is_stereo;
        QRgb color;
        double thickness;
    };

    ScopeRenderer(const QString& filename,
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
        double x;
        double y;
        double w;
        double h;
        bool is_stereo;
        QRgb color;
        double thickness;
    };

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
                    double mid_y,
                    double thickness,
                    QRgb color);
};

Q_DECLARE_METATYPE(ScopeRenderer::GlobalArgs)
Q_DECLARE_METATYPE(ScopeRenderer::ChannelArgs)

#endif // SCOPERENDERER_H
