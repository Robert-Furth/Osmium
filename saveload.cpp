#include "saveload.h"

#include <filesystem>
#include <fstream>
#include <string>

#include <QStandardPaths>
#include <QString>

#include <toml.hpp>

namespace fs = std::filesystem;

fs::path config_path() {
    auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        return "";
    dir.append("/config.toml");
    return dir.toStdString();
}

bool PersistentConfig::load() {
    auto load_path = config_path();
    if (!fs::exists(load_path))
        return false;

    toml::table table;
    try {
        table = toml::parse_file(load_path.string());
    } catch (const toml::parse_error& e) {
        return false;
    }

    auto settings = table["paths"];

    auto sf_path = settings["soundfont_path"].value<std::string>();
    if (sf_path)
        soundfont_path = *sf_path;

    auto in_dir = settings["input_file_dir"].value<std::string>();
    if (in_dir)
        input_file_dir = *in_dir;

    auto out_dir = settings["output_file_dir"].value<std::string>();
    if (out_dir)
        output_file_dir = *out_dir;

    return true;
}

bool PersistentConfig::save() {
    auto save_path = config_path();
    if (save_path.empty())
        return false;

    fs::create_directories(save_path.parent_path());

    auto table = toml::table{
        {"paths",
         toml::table{
             {"soundfont_path", soundfont_path},
             {"input_file_dir", input_file_dir},
             {"output_file_dir", output_file_dir},
         }},
    };

    std::ofstream os(save_path);
    os << table << "\n";
    bool ok = os.good();
    os.close();
    return ok;
}
