#include "eventtracker.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include <bass.h>
#include <bassmidi.h>

#include "error.h"
#include "handlewrapper.h"

namespace osmium {

EventTracker::EventTracker(uint32_t raw_handle, uint32_t fps) : m_s_per_frame(1.0 / fps) {
    if (!raw_handle)
        throw Error::from_bass_error("Error creating EventTracker: ");

    HandleWrapper handle(raw_handle);

    int32_t num_events = BASS_MIDI_StreamGetEvents(*handle, -1, 0, nullptr);
    if (num_events == -1)
        throw Error::from_bass_error();
    m_events.resize(num_events);
    m_times.reserve(num_events);
    if (BASS_MIDI_StreamGetEvents(*handle, -1, 0, m_events.data()) == -1)
        throw Error::from_bass_error();

    // Calculate timestamps for each event
    float ticks_per_qn_f;
    if (!BASS_ChannelGetAttribute(*handle, BASS_ATTRIB_MIDI_PPQN, &ticks_per_qn_f))
        throw Error::from_bass_error("Could not get PPQN attribute: ");
    double qn_per_tick = 1.0 / ticks_per_qn_f;

    // seconds per tick = us/qn * qn/tick * 1000000
    // default tempo is 120bpm = 500000 us/qn
    double s_per_tick = 0.5 * qn_per_tick;
    double time_of_last_tempo_change = 0;
    uint32_t tick_of_last_tempo_change = 0;
    double cur_seconds = 0;
    for (const auto& event : m_events) {
        cur_seconds = time_of_last_tempo_change
                      + (event.tick - tick_of_last_tempo_change) * s_per_tick;

        if (event.event == MIDI_EVENT_TEMPO) {
            s_per_tick = event.param * qn_per_tick * 1e-6;
            time_of_last_tempo_change = cur_seconds;
            tick_of_last_tempo_change = event.tick;
        }

        m_times.push_back(cur_seconds);
    }
}

EventTracker::EventTracker(const char* filename, uint32_t fps)
    : EventTracker(BASS_MIDI_StreamCreateFile(false, filename, 0, 0, BASS_STREAM_DECODE, 0),
                   fps) {}

void EventTracker::next_events() {
    m_event_window.clear();

    if (m_event_index >= m_events.size())
        return;

    m_cur_frame++;
    double seconds = m_cur_frame * m_s_per_frame;

    size_t i;
    for (i = m_event_index; i < m_times.size(); i++) {
        if (m_times[i] < seconds) {
            m_event_window.emplace_back(m_events[i]);
        } else {
            break;
        }
    }
    m_event_index = i;
}

/*void get_events(const char* filename) {
    auto raw_handle
        = BASS_MIDI_StreamCreateFile(false, filename, 0, 0, BASS_STREAM_DECODE, 0);
    if (!raw_handle)
        return;
    HandleWrapper handle(raw_handle);

    uint32_t num_events = BASS_MIDI_StreamGetEvents(*handle, -1, 0, nullptr);
    std::vector<BASS_MIDI_EVENT> events(num_events);
    BASS_MIDI_StreamGetEvents(*handle, -1, 0, events.data());

    float ppqn; // ticks per quarter note
    if (!BASS_ChannelGetAttribute(*handle, BASS_ATTRIB_MIDI_PPQN, &ppqn))
        return;

    uint32_t uspqn = 500000; // microseconds per quarter note; defaults to 500000us = 120bpm

    // us/qn * qn/tick = us/tick
    double s_per_tick = static_cast<double>(uspqn / ppqn) * 0.000001;
    double time_of_last_tempo_change = 0;
    uint32_t tick_of_last_tempo_change = 0;
    double cur_seconds = 0;
    double cur_seconds_2 = 0;

    std::cout << std::setprecision(24);
    for (const auto& event : events) {
        // I don't really like this, since repeatedly adding doubles together can lead to drift
        cur_seconds = time_of_last_tempo_change
                      + (event.tick - tick_of_last_tempo_change) * s_per_tick;

        if (event.event == MIDI_EVENT_TEMPO) {
            uspqn = event.param;
            s_per_tick = static_cast<double>(uspqn / ppqn) * 0.000001;
            time_of_last_tempo_change = cur_seconds;
            tick_of_last_tempo_change = event.tick;
        }
        std::cout << std::setprecision(24) << std::dec << std::setprecision(3)
                  << cur_seconds << "\t" << cur_seconds_2 << "\tT" << event.tick << "\tCH"
                  << event.chan << "\t" << event.event << "(" << std::hex << event.param
                  << ")\n";
    }
}*/

// std::vector<std::string> get_track_names(const char* filename) {}

} // namespace osmium
