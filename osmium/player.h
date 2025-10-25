#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>
#include <vector>

#include "handlewrapper.h"

namespace osmium {

class Player {
public:
    Player(uint32_t handle, uint32_t fps);
    Player(const char* filename, uint32_t fps);

    const std::vector<float>& get_samples() const { return m_buffer; }
    uint32_t get_sample_rate() const { return m_sample_rate; }
    uint32_t get_num_channels() const { return m_num_channels; }
    bool is_playing() const;
    void next_wave_data();

private:
    HandleWrapper m_stream_handle;
    uint32_t m_samples_per_frame;
    uint32_t m_sample_rate;
    uint32_t m_num_channels;

    std::vector<float> m_buffer_full;
    std::vector<float> m_buffer;
};

} // namespace osmium

#endif // PLAYER_H
