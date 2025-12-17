#ifndef RENDERARGS_H
#define RENDERARGS_H

#include <QFont>
#include <QObject>
#include <QRgb>

#include "config.h"

enum class ChannelOrder { ROW_MAJOR, COLUMN_MAJOR };

struct GlobalArgs {
    int width;
    int height;
    int num_rows_or_cols;
    ChannelOrder order;

    int fps;
    double volume;
    VideoCodec vid_codec;
    H26xPreset h26x_preset;
    int crf;

    int bitrate_kbps;

    QRgb border_color;
    double border_thickness;
    QRgb background_color;
    bool debug_vis;
};

struct ChannelArgs {
    int channel_number;

    int scope_width_ms;
    double amplification;
    bool is_stereo;

    QRgb color;
    double thickness;
    QRgb midline_color;
    double midline_thickness;
    bool draw_h_midline;
    bool draw_v_midline;

    bool draw_labels;
    QString label_template;
    QFont label_font;
    QRgb label_color;

    int max_nudge_ms;
    double trigger_threshold;
    double similarity_bias;
    int similarity_window_ms;
    double peak_bias;
    double peak_threshold;
    double drift_window_ms;
    double avoid_drift_bias;
};

Q_DECLARE_METATYPE(GlobalArgs)
Q_DECLARE_METATYPE(ChannelArgs)

#endif // RENDERARGS_H
