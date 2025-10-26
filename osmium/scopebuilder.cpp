#include "scopebuilder.h"

#include <format>
#include <string>

#include <bass.h>
#include <bassmidi.h>

#include "error.h"

namespace osmium {

// -- Callbacks --

BOOL CALLBACK midi_filter_channel(
    HSTREAM handle, int track, BASS_MIDI_EVENT* event, BOOL seeking, void* user) {
    int* channelp = static_cast<int*>(user);
    return event->chan == *channelp;
}

// -- Helpers --

std::vector<HSOUNDFONT> construct_soundfonts(const std::vector<std::string>& soundfonts) {
    std::vector<HSOUNDFONT> handles;
    for (const auto& filename : soundfonts) {
        HSOUNDFONT handle = BASS_MIDI_FontInit(filename.c_str(), 0);
        if (!handle)
            throw Error::from_bass_error("Error initializing soundfont: ");

        handles.push_back(handle);
    }

    return handles;
}

// -- ScopeBuilder --

Scope ScopeBuilder::build_from_file(const char* filename) {
    uint32_t flags = BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE;
    if (!m_stereo) {
        flags |= BASS_SAMPLE_MONO;
    }

    HSTREAM handle = BASS_StreamCreateFile(BASS_FILE_NAME, filename, 0, 0, flags);
    if (!handle) {
        std::string msg = std::format("Error opening file {}: ", filename);
        throw Error::from_bass_error(msg);
    }

    return build_from_handle(handle);
}

Scope ScopeBuilder::build_from_midi_channel(const char* filename, int channel) {
    uint32_t flags = BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | BASS_MIDI_DECAYEND;
    if (!m_stereo) {
        flags |= BASS_SAMPLE_MONO;
    }

    HSTREAM handle = BASS_MIDI_StreamCreateFile(false, filename, 0, 0, flags, 0);
    if (!handle)
        throw Error::from_bass_error("Error creating stream: ");

    Scope scope = build_from_handle(handle);
    scope.m_stream_handle.set_extra_data(channel);

    int result = BASS_MIDI_StreamSetFilter(handle,
                                           false,
                                           midi_filter_channel,
                                           scope.m_stream_handle.extra_data_ptr().get());
    if (!result)
        throw Error::from_bass_error("Error creating filter: ");

    return scope;
}

Scope ScopeBuilder::build_from_handle(HSTREAM handle) {
    BASS_CHANNELINFO info;
    BASS_ChannelGetInfo(handle, &info);
    if (!BASS_ChannelGetInfo(handle, &info))
        throw Error::from_bass_error("Error getting channel info: ");

    uint32_t samples_per_frame = info.freq / m_frame_rate;
    // samples/sec * ms/window * sec/ms = samples/window
    uint32_t samples_per_window = info.freq * m_display_window_ms / 1000;
    uint32_t max_nudge_samples = info.freq * m_max_nudge_ms / 1000;

    Scope scope(handle,
                samples_per_frame,
                info.freq,
                samples_per_window,
                max_nudge_samples,
                m_amplification,
                m_trigger_threshold,
                info.chans,
                m_stereo);

    if (!m_soundfonts.empty()) {
        auto sf_handles = construct_soundfonts(m_soundfonts);
        scope.m_stream_handle.set_soundfonts(sf_handles);
    }

    return scope;
}

} // namespace osmium
