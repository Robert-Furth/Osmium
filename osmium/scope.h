#ifndef SCOPE_H
#define SCOPE_H

#include <cstdint>
#include <optional>
#include <vector>

#include "handlewrapper.h"

namespace osmium {

class Scope {
public:
    const std::vector<float>& get_left_samples() const { return m_left_output; }
    const std::vector<float>& get_right_samples() const { return m_right_output; }

    uint64_t get_current_progress() const { return m_total_samples_read; }
    uint32_t get_sample_rate() const { return m_sample_rate; }
    double get_window_size_ms() const { return 1000.0 * m_window_size / m_sample_rate; }
    double get_max_nudge_ms() const { return 1000.0 * m_max_nudge / m_sample_rate; }
    double get_this_nudge_ms() const { return 1000.0 * m_nudge_amount / m_sample_rate; }

    uint64_t get_total_samples() const;
    bool is_playing() const;

    void next_wave_data();

private:
    HandleWrapper m_stream_handle;

    uint32_t m_samples_per_frame;
    uint32_t m_sample_rate;
    uint32_t m_window_size;
    uint32_t m_max_nudge;
    float m_amplification;
    float m_trigger_threshold;
    unsigned m_src_num_channels;
    bool m_is_stereo;

    uint64_t m_total_samples_read = 0;
    int32_t m_nudge_amount = 0;
    int m_frame_num = 0;

    std::vector<float> m_left_output;
    std::vector<float> m_right_output;
    std::vector<float> m_left_buffer;
    std::vector<float> m_right_buffer;

    Scope(uint32_t handle,
          uint32_t samples_per_frame,
          uint32_t sample_rate,
          uint32_t window_size,
          uint32_t max_nudge,
          float amplification,
          float trigger_threshold,
          unsigned src_num_channels,
          bool is_stereo);

    void update_buffers();

    std::optional<size_t> find_best_startpoint(const std::vector<float>&,
                                               const std::vector<float>&);

    friend class ScopeBuilder;
};

} // namespace osmium

#endif // SCOPE_H
