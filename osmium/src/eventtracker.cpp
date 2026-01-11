#include "eventtracker.h"

#include <cstdint>
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

    uint32_t num_events = BASS_MIDI_StreamGetEvents(*handle, -1, 0, nullptr);
    if (num_events == -1)
        throw Error::from_bass_error();
    m_events.resize(num_events);
    m_times.reserve(num_events);

    // Make sure that osmium::Event can be reinterpret_cast'ed to BASS_MIDI_EVENT safely
    // Can't use std::is_layout_compatible because DWORDs are either unsigned long or
    // uint32_t (unsigned int) depending on OS
    static_assert(sizeof(Event) == sizeof(BASS_MIDI_EVENT)
                  && alignof(Event) == alignof(BASS_MIDI_EVENT));

    uint32_t result = BASS_MIDI_StreamGetEvents(
        *handle, -1, 0, reinterpret_cast<BASS_MIDI_EVENT*>(m_events.data()));
    if (result == -1)
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

        if (event.event == Event::Tempo) {
            s_per_tick = event.param * qn_per_tick * 1e-6;
            time_of_last_tempo_change = cur_seconds;
            tick_of_last_tempo_change = event.tick;
        }

        m_times.push_back(cur_seconds);
    }
}

EventTracker::EventTracker(const char* filename, uint32_t fps)
    : EventTracker(
          BASS_MIDI_StreamCreateFile(false, filename, 0, 0, BASS_STREAM_DECODE, 0), fps) {
}

void EventTracker::next_events() {
    m_event_window.clear();

    if (m_event_index >= m_events.size())
        return;

    m_cur_frame++;
    double seconds = m_cur_frame * m_s_per_frame;

    size_t i;
    for (i = m_event_index; i < m_times.size() && m_times[i] <= seconds; i++) {
        m_event_window.emplace_back(m_events[i]);
    }
    m_event_index = i;
}

const std::vector<bool>& EventTracker::get_channels_with_notes() {
    if (!m_channel_has_notes.empty())
        return m_channel_has_notes;

    for (const auto& event : m_events) {
        if (event.chan >= m_channel_has_notes.size()) {
            m_channel_has_notes.resize(event.chan + 1);
        }

        if (event.event == Event::EventType::Note) {
            m_channel_has_notes[event.chan] = true;
        }
    }

    return m_channel_has_notes;
}

} // namespace osmium
