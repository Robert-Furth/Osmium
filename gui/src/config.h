#ifndef CONFIG_H
#define CONFIG_H

#include <filesystem>

#include <QString>

enum class VideoCodec {
// I dislike X-macros, but I dislike repeating myself more.
#define X(f, s) f,
#include "xmacro/video_codec.txt"
#undef X
};

enum class H26xPreset {
#define X(f, s) f,
#include "xmacro/h26x_preset.txt"
#undef X
};

VideoCodec video_codec(const std::string& key, VideoCodec default_val);
H26xPreset h26x_preset(const std::string&, H26xPreset default_val);
QString to_string(VideoCodec);
QString to_string(H26xPreset);

struct PathConfig {
    QString soundfont_path;
    bool use_system_ffmpeg;
    QString ffmpeg_path;
    QString input_file_dir;
    QString output_file_dir;
};

struct VideoConfig {
    VideoCodec codec;
    H26xPreset h26x_preset;
    int h26x_crf;
};

struct AudioConfig {
    int bitrate_kbps;
};

struct PersistentConfig {
    PathConfig path_config;
    VideoConfig video_config;
    AudioConfig audio_config;
};

PersistentConfig load_config();
PersistentConfig load_config(const std::filesystem::path& path);

bool save_config(const PersistentConfig&);
bool save_config(const PersistentConfig&, const std::filesystem::path& path);

#endif // CONFIG_H
