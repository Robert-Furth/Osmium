#ifndef CONFIG_H
#define CONFIG_H

#include <filesystem>
#include <string>

enum class VideoCodec {
// I dislike X-macros, but I dislike repeating myself more.
#define X(f, s) f,
#include "xmacro/video_codec.txt"
#undef X
    Invalid,
};

enum class H26xPreset {
#define X(f, s) f,
#include "xmacro/h26x_preset.txt"
#undef X
    Invalid,
};

VideoCodec video_codec(const std::string&);
H26xPreset h26x_preset(const std::string&);
std::string to_string(VideoCodec);
std::string to_string(H26xPreset);

struct PathConfig {
    std::string soundfont_path;
    bool use_system_ffmpeg;
    std::string ffmpeg_path;
    std::string input_file_dir;
    std::string output_file_dir;
};

struct VideoConfig {
    VideoCodec codec;
    H26xPreset preset;
    int crf;
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
