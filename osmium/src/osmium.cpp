#include "osmium.h"

#include <iostream>

#include <bass.h>
#include <bassmidi.h>

namespace osmium {

static HPLUGIN g_midi_plugin = 0;

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

} // namespace osmium
