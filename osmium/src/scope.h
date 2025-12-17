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
    bool get_no_nudges_found() const { return m_no_good_nudge; }

    uint64_t get_total_samples() const;
    bool is_playing() const;

    void next_wave_data();

private:
    HandleWrapper m_stream_handle;

    int32_t m_samples_per_frame;
    uint32_t m_sample_rate;
    int32_t m_window_size;
    double m_amplification;
    unsigned m_src_num_channels;
    bool m_is_stereo;

    // Inter-frame alignment
    int32_t m_max_nudge;           // How far forward a sample can be moved to fit
    int32_t m_similarity_window;   // # samples to check for similarity between frames
    double m_trigger_threshold;    // % of peak amplitude to trigger at
    double m_similarity_bias;      // How much to consider inter-frame similarity
    double m_peak_bias;            // How much to consider pre-peak zero-crosses
    double m_peak_bias_min_factor; // % of peak amplitude to count as pre-peak
    int32_t m_drift_window;        // # samples around a zero-cross to consider
    double m_avoid_drift_bias;     // How much to penalize samples far from a zero-cross

    // Info/debug variables
    uint64_t m_total_samples_read = 0;
    int32_t m_nudge_amount = 0;
    int32_t m_nudge_change = 0;
    int m_frame_num = 0;
    bool m_no_good_nudge = false;

    // Buffers
    std::vector<float> m_left_output;
    std::vector<float> m_right_output;
    std::vector<float> m_left_buffer;
    std::vector<float> m_right_buffer;

    Scope(uint32_t handle, uint32_t window_size, uint32_t internal_size);

    void update_buffers();

    std::optional<int32_t> find_best_nudge(const std::vector<float>&,
                                           const std::vector<float>&);

    friend class ScopeBuilder;
};

} // namespace osmium

#endif // SCOPE_H
