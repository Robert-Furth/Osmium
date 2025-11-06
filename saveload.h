#ifndef SAVELOAD_H
#define SAVELOAD_H

#include <string>

struct PersistentConfig {
    std::string soundfont_path;
    std::string input_file_dir;
    std::string output_file_dir;

    bool save();
    bool load();
    void init_default();
};

#endif // SAVELOAD_H
