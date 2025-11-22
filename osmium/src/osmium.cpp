#include "osmium.h"

#include <iostream>

#include <bass.h>
#include <bassmidi.h>

namespace osmium {

static HPLUGIN g_midi_plugin = 0;
// static std::vector<HSOUNDFONT> g_loaded_fonts;

bool init() {
    g_midi_plugin = BASS_PluginLoad("bassmidi", 0);
    if (!g_midi_plugin) {
        std::cout << "Error initializing bassmidi (code " << BASS_ErrorGetCode() << ")\n";
        return false;
    }

    // TODO: User-configurable sample rate?
    return BASS_Init(0, 48000, BASS_DEVICE_STEREO, 0, nullptr);
}

void uninit() {
    BASS_PluginFree(g_midi_plugin);
    BASS_Free();
}

// bool set_soundfonts(const std::vector<std::string> soundfonts) {
//     // Unload loaded fonts
//     for (HSOUNDFONT handle : g_loaded_fonts) {
//         BASS_MIDI_FontFree(handle);
//     }
//     g_loaded_fonts.clear();

//     // Create handles for each font
//     for (const auto& filepath : soundfonts) {
//         HSOUNDFONT handle = BASS_MIDI_FontInit(filepath.c_str(), 0);
//         if (!handle)
//             return false;
//         g_loaded_fonts.push_back(handle);
//     }

//     //
//     std::vector<BASS_MIDI_FONT> fonts;
//     for (HSOUNDFONT handle : g_loaded_fonts) {
//         fonts.push_back({handle, 0, 0});
//     }
//     return BASS_MIDI_StreamSetFonts(0, fonts.data(), fonts.size());
// }

} // namespace osmium
