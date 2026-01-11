#ifndef EVENTTRACKER_H
#define EVENTTRACKER_H

#include <cstdint>
#include <vector>

namespace osmium {

// Should be compatible with BASS_MIDI_EVENT from bassmidi.h.
// (Annoyingly, BASS uses DWORDs, which are typedef'd to unsigned long on Windows but
// unsigned int (via uint32_t) on other OSes. This makes conversion annoying.)
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
    const std::vector<bool>& get_channels_with_notes();

private:
    std::vector<Event> m_event_window;
    std::vector<Event> m_events;
    std::vector<double> m_times;
    std::vector<bool> m_channel_has_notes;

    size_t m_event_index = 0;
    unsigned int m_cur_frame = 0;
    double m_s_per_frame;
};

} // namespace osmium

#endif // EVENTTRACKER_H
