#include "config.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

#include <QStandardPaths>
#include <QString>

#define TOML_EXCEPTIONS 0

#include <toml.hpp>

namespace fs = std::filesystem;

VideoCodec video_codec(const std::string& key, VideoCodec default_val) {
    static const std::unordered_map<std::string, VideoCodec> codec_map{
#define X(f, s) {s, VideoCodec::f},
#include "xmacro/video_codec.txt"
#undef X
    };

    return codec_map.contains(key) ? codec_map.at(key) : default_val;
}

H26xPreset h26x_preset(const std::string& key, H26xPreset default_val) {
    static const std::unordered_map<std::string, H26xPreset> preset_map{
#define X(f, s) {s, H26xPreset::f},
#include "xmacro/h26x_preset.txt"
#undef X
    };

    return preset_map.contains(key) ? preset_map.at(key) : default_val;
}

QString to_string(VideoCodec codec) {
    switch (codec) {
#define X(f, s) \
    case VideoCodec::f: \
        return s;
#include "xmacro/video_codec.txt"
#undef X

    default:
        return "";
    }
}

QString to_string(H26xPreset preset) {
    switch (preset) {
#define X(f, s) \
    case H26xPreset::f: \
        return s;
#include "xmacro/h26x_preset.txt"
#undef X

    default:
        return "";
    }
}

fs::path config_path() {
    auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        return "";
    dir.append("/config.toml");
    return dir.toStdString();
}

PathConfig load_path_config(const toml::node_view<toml::node>& v) {
    auto x = QString::fromStdString(v["soundfont_path"].value_or<std::string>(""));
    return PathConfig{
        .soundfont_path = QString::fromStdString(
            v["soundfont_path"].value_or<std::string>("")),
        .use_system_ffmpeg = v["use_system_ffmpeg"].value_or(true),
        .ffmpeg_path = QString::fromStdString(v["ffmpeg_path"].value_or<std::string>("")),
        .input_file_dir = QString::fromStdString(
            v["input_file_dir"].value_or<std::string>("")),
        .output_file_dir = QString::fromStdString(
            v["output_file_dir"].value_or<std::string>("")),
    };
}

VideoConfig load_video_config(const toml::node_view<toml::node>& v) {
    static const std::unordered_map<std::string, VideoCodec> codec_map{
        {"h264", VideoCodec::H264},
        {"h265", VideoCodec::H265},
    };

    std::string codec_str = v["codec"].value_or("");
    VideoCodec codec = video_codec(codec_str, VideoCodec::H264);

    std::string preset_str = v["h26x_preset"].value_or("");
    H26xPreset preset = h26x_preset(preset_str, H26xPreset::Medium);

    int crf = v["h26x_crf"].value_or(codec == VideoCodec::H264 ? 23 : 28);
    crf = std::clamp(crf, 0, 51);

    return VideoConfig{
        .codec = codec,
        .h26x_preset = preset,
        .h26x_crf = crf,
    };
}

AudioConfig load_audio_config(const toml::node_view<toml::node>& v) {
    int bitrate = v["bitrate"].value_or(192);
    bitrate = std::clamp(bitrate, 128, 256);

    return AudioConfig{
        .bitrate_kbps = bitrate,
    };
}

PersistentConfig load_config() {
    return load_config(config_path());
}

PersistentConfig load_config(const fs::path& load_path) {
    auto table = toml::parse_file(load_path.c_str());

    auto paths = table["paths"];
    if (!paths) {
        paths = table["osmium"];
    }

    return PersistentConfig{
        .path_config = load_path_config(paths),
        .video_config = load_video_config(table["video"]),
        .audio_config = load_audio_config(table["audio"]),
    };
}

bool save_config(const PersistentConfig& config) {
    return save_config(config, config_path());
}

bool save_config(const PersistentConfig& config, const fs::path& save_path) {
    if (save_path.empty())
        return false;

    fs::create_directories(save_path.parent_path());

    toml::table table{
        {"paths",
         toml::table{
             {"soundfont_path", config.path_config.soundfont_path.toStdString()},
             {"use_system_ffmpeg", config.path_config.use_system_ffmpeg},
             {"ffmpeg_path", config.path_config.ffmpeg_path.toStdString()},
             {"input_file_dir", config.path_config.input_file_dir.toStdString()},
             {"output_file_dir", config.path_config.output_file_dir.toStdString()},
         }},

        {"video",
         toml::table{
             {"codec", to_string(config.video_config.codec).toStdString()},
             {"h26x_preset", to_string(config.video_config.h26x_preset).toStdString()},
             {"h26x_crf", config.video_config.h26x_crf},
         }},

        {"audio",
         toml::table{
             {"bitrate", config.audio_config.bitrate_kbps},
         }},
    };

    std::ofstream os(save_path);
    os << table << "\n";
    bool ok = os.good();
    os.close();
    return ok;
}
