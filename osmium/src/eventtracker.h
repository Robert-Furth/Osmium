#ifndef EVENTTRACKER_H
#define EVENTTRACKER_H

#include <cstdint>
#include <vector>

namespace osmium {

// Compatible with BASS_MIDI_EVENT from bassmidi.h
struct Event {
    // Warning: extremely incomplete!
    enum EventType : uint32_t {
        Note = 1,
        Program = 2,
        Bank = 10,
        Tempo = 62,
    };

    EventType event;
    uint32_t param;
    uint32_t chan;
    uint32_t tick;
    uint32_t pos;
};

class EventTracker {
public:
    EventTracker(uint32_t raw_handle, uint32_t fps);
    EventTracker(const char* filename, uint32_t fps);

    void next_events();
    const std::vector<Event>& get_events() const { return m_event_window; }

private:
    std::vector<Event> m_event_window;
    std::vector<Event> m_events;
    std::vector<double> m_times;

    size_t m_event_index = 0;
    unsigned int m_cur_frame = 0;
    double m_s_per_frame;
};

} // namespace osmium

#endif // EVENTTRACKER_H
