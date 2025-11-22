#include "scopebuilder.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <algorithm>
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
    // Filter out all notes not from the given channel
    if (event->event == MIDI_EVENT_NOTE)
        return event->chan == *channelp;
    return true;
}

BOOL CALLBACK midi_filter_track(
    HSTREAM handle, int track, BASS_MIDI_EVENT* event, BOOL seeking, void* user) {
    int* channelp = static_cast<int*>(user);
    // Filter out all notes not from the given track
    if (event->event == MIDI_EVENT_NOTE)
        return track == *channelp;
    return true;
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
    uint32_t similarity_window = info.freq * m_similarity_window_ms / 1000;

    Scope scope(handle, samples_per_window, samples_per_window + max_nudge_samples);
    scope.m_samples_per_frame = samples_per_frame;
    scope.m_sample_rate = info.freq;
    scope.m_window_size = samples_per_window;
    scope.m_amplification = m_amplification;
    scope.m_src_num_channels = info.chans;
    scope.m_is_stereo = m_stereo;

    scope.m_max_nudge = max_nudge_samples;
    scope.m_trigger_threshold = m_trigger_threshold;
    scope.m_similarity_bias = m_similarity_bias;
    scope.m_similarity_window = std::min(similarity_window, samples_per_window);
    scope.m_peak_bias = m_peak_bias;
    scope.m_peak_bias_min_factor = m_peak_threshold;

    if (!m_soundfonts.empty()) {
        auto sf_handles = construct_soundfonts(m_soundfonts);
        scope.m_stream_handle.set_soundfonts(sf_handles);
    }

    return scope;
}

} // namespace osmium
