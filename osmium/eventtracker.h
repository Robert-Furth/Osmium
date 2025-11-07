#ifndef EVENTTRACKER_H
#define EVENTTRACKER_H

#include <cstdint>
#include <string>
#include <vector>

#include <bassmidi.h>

namespace osmium {

class EventTracker {
public:
    EventTracker(uint32_t handle, uint32_t fps);
    EventTracker(const char* filename, uint32_t fps);

    void next_events();
    const std::vector<BASS_MIDI_EVENT> get_events() const { return m_event_window; }

private:
    // HandleWrapper m_stream_handle;
    std::vector<BASS_MIDI_EVENT> m_event_window;
    std::vector<BASS_MIDI_EVENT> m_events;
    std::vector<double> m_times;

    size_t m_event_index = 0;
    unsigned int m_cur_frame = 0;
    double m_s_per_frame;
};

// void get_events(const char* filename);

std::vector<std::string> get_track_names(const char* filename);

} // namespace osmium

#endif // EVENTTRACKER_H
