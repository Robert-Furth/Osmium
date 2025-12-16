#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "scope.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <vector>

#include <bass.h>

#include "error.h"

namespace osmium {

// --- helper functions ---

namespace {

/** "Left-shifts" `src` 's elements into `dest`.
 *  If `src.size() < min_size`, shifts in enough copies of `default_val` until
 *  `min_size` values have been shifted (as if `src` had been padded with
 *  `default_val` until it reached `min_size`).
 */
template<typename T>
void shift_in(std::vector<T>& dest,
              const std::vector<T>& src,
              size_t min_size,
              T default_val = 0) {
    size_t size = std::max(min_size, src.size());
    size_t src_offset = src.size() > dest.size() ? src.size() - dest.size() : 0;

    // If `dest` won't be completely overwritten, shift back the existing values
    // in `dest`. `std::copy()` returns an iterator pointing to the element just
    // after the copied data.
    auto it = size >= dest.size()
                  ? dest.begin()
                  : std::copy(dest.begin() + size, dest.end(), dest.begin());

    // Copy the values from `src` to the point in `dest` just after `std::copy()`
    // finished, then fill the remainder with `default_val`
    it = std::copy(src.cbegin() + src_offset, src.cend(), it);
    std::fill(it, dest.end(), default_val);
}

/** Returns the integer `n` in range [0, `m`) s.t. `a` ≡ `n` (mod `m`) */
constexpr int32_t mod(int32_t a, uint32_t m) {
    while (a < 0)
        a += m;
    return a % m;
}

constexpr bool mod_within_window(int32_t n, int32_t min, int32_t max) {
    if (min <= max)
        return min <= n && n <= max;
    return n <= max || min <= n;
}

template<std::forward_iterator It>
It abs_max_element(It begin, It end) {
    It it = begin;
    It candidate = end;
    typename std::iterator_traits<It>::value_type candidate_val;

    for (; it < end; ++it) {
        if (std::abs(*it) > candidate_val) {
            candidate_val = std::abs(*it);
            candidate = it;
        }
    }

    return candidate;
}

template<typename T>
std::vector<T> stereo_downmix(const std::vector<T>& left, const std::vector<T>& right) {
    std::vector<T> result(std::min(left.size(), right.size()), 0);
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = (left[i] + right[i]) / 2;
    }
    return result;
}

} // namespace

// --- osmium::Scope implementation ---

Scope::Scope(uint32_t handle, uint32_t window_size, uint32_t internal_size)
    : m_stream_handle(handle) {
    m_left_output.resize(window_size);
    m_left_buffer.resize(internal_size);
    m_right_output.resize(window_size);
    m_right_buffer.resize(internal_size);
}

void Scope::next_wave_data() {
    update_buffers();

    std::optional<int32_t> maybe_nudge;
    if (m_is_stereo) {
        maybe_nudge = find_best_nudge(stereo_downmix(m_left_buffer, m_right_buffer),
                                      stereo_downmix(m_left_output, m_right_output));
    } else {
        maybe_nudge = find_best_nudge(m_left_buffer, m_right_buffer);
    }

    m_no_good_nudge = !maybe_nudge.has_value();
    int32_t nudge = maybe_nudge.value_or(0);
    m_nudge_change = nudge - m_nudge_amount;
    m_nudge_amount = nudge;

    for (uint32_t i = 0; i < m_window_size; i++) {
        m_left_output[i] = m_left_buffer[i + nudge] * m_amplification;
    }
    if (m_is_stereo) {
        for (uint32_t i = 0; i < m_window_size; i++) {
            m_right_output[i] = m_right_buffer[i + nudge] * m_amplification;
        }
    }
}

void Scope::update_buffers() {
    std::vector<float> data(static_cast<size_t>(m_samples_per_frame) * m_src_num_channels);

    // Read the stream data
    uint32_t bytes_read = BASS_ChannelGetData(*m_stream_handle,
                                              data.data(),
                                              (data.size() * sizeof(float))
                                                  | BASS_DATA_FLOAT);
    if (bytes_read == -1) {
        int code = BASS_ErrorGetCode();
        if (code != BASS_ERROR_ENDED)
            throw Error::from_bass_error("Error getting wave data: ");

        // At the end of the data; shift in zeroes and exit early
        shift_in(m_left_buffer, {}, m_samples_per_frame);
        shift_in(m_right_buffer, {}, m_samples_per_frame);
        return;
    }

    m_frame_num++;
    uint32_t samples_read = bytes_read / sizeof(float) / m_src_num_channels;
    m_total_samples_read += static_cast<uint64_t>(samples_read) * m_src_num_channels;

    // Demux the data into left and right channels
    std::vector<float> new_left_samples(samples_read);
    std::vector<float> new_right_samples(samples_read);

    if (m_is_stereo) {
        if (m_src_num_channels == 1) {
            std::copy(data.cbegin(),
                      data.cbegin() + samples_read,
                      new_left_samples.begin());
        } else {
            // TODO: Maybe try downmixing 5.1/7.1 to stereo? Probably overkill tbh.
            for (uint32_t i = 0; i < samples_read; i++) {
                uint32_t j = i * m_src_num_channels;
                new_left_samples[i] = data[j];
                new_right_samples[i] = data[j + 1];
            }
        }
    } else {
        if (m_src_num_channels == 1) {
            std::copy(data.cbegin(),
                      data.cbegin() + samples_read,
                      new_left_samples.begin());
        } else {
            for (uint32_t i = 0; i < samples_read; i++) {
                uint32_t j = i * m_src_num_channels;
                new_left_samples[i] = (data[j] + data[j + 1]) * 0.5;
            }
        }
    }

    // Shift the samples into their respective buffers.
    shift_in(m_left_buffer, new_left_samples, m_samples_per_frame);
    if (m_is_stereo) {
        shift_in(m_right_buffer, new_right_samples, m_samples_per_frame);
    }
}

std::optional<int32_t> Scope::find_best_nudge(const std::vector<float>& floats,
                                              const std::vector<float>& prev) {
    const double EPSILON = 0.005;

    // floats.size() == m_window_size + m_max_nudge;
    uint32_t view_window_midpoint = m_window_size / 2;

    // double peak_amplitude = *abs_max_element(floats.cbegin(), floats.cend());
    double peak_amplitude = *std::max_element(floats.cbegin(), floats.cend());
    double peak_amp_threshold = peak_amplitude * m_peak_bias_min_factor;
    double trigger_threshold = std::max(EPSILON, m_trigger_threshold * peak_amplitude);

    // Find all rising edges within the window
    std::vector<int32_t> nudges;
    std::vector<int32_t> peak_nudges;
    int32_t last_candidate_nudge = 0;
    float last_amplitude = 0;

    bool lower_bound_cross = false;
    bool upper_bound_cross = false;

    for (int32_t offs = 0; offs < m_max_nudge; offs++) {
        float f = floats[view_window_midpoint + offs];

        if (f < -trigger_threshold) {
            lower_bound_cross = false;
        } else if (last_amplitude < -trigger_threshold) {
            lower_bound_cross = true;
        }

        if (f < trigger_threshold) {
            upper_bound_cross = false;
        } else if (last_amplitude < trigger_threshold) {
            upper_bound_cross = true;
        }

        if (last_amplitude < EPSILON && f >= EPSILON) {
            last_candidate_nudge = offs;
        }

        if (lower_bound_cross && upper_bound_cross) {
            nudges.push_back(last_candidate_nudge);
            lower_bound_cross = false;
            upper_bound_cross = false;
        }

        if (f > peak_amp_threshold && !nudges.empty()
            && (peak_nudges.empty() || peak_nudges.back() != nudges.back())) {
            peak_nudges.push_back(last_candidate_nudge);
        }

        last_amplitude = f;
    }

    // Early exits if 0 or 1 candidate nudges were found
    if (nudges.size() == 0)
        return std::nullopt;
    if (nudges.size() == 1)
        return nudges[0];

    // Compute an error value for each candidate nudge. This is based on
    // multiple factors that can be weighted individually by the user.

    uint32_t sim_window_start = (m_window_size - m_similarity_window) / 2;
    size_t peak_nudge_idx = 0;
    int32_t best_nudge = 0;
    double min_error = std::numeric_limits<double>::infinity();

    // For debug visualization
    int32_t other_best_nudge = 0;
    double other_min_error = std::numeric_limits<double>::infinity();

    for (auto nudge : nudges) {
        double errors[] = {0, 0};
        bool flag2 = false;

        // Factor 1: Compute the average difference between a candidate view
        // window and the previous one. (Should typically range between 0 and 2)
        for (uint32_t i = 0; i < m_similarity_window; i++) {
            errors[0] += std::abs(floats[nudge + i + sim_window_start]
                                  - prev[i + sim_window_start]);
        }
        errors[0] = errors[0] / m_similarity_window;

        // Factor 2: If the difference between this nudge and the last nudge is
        // close to the difference between the last nudge and the one before it,
        // multiply a bias factor with the error.
        //   This prioritizes moving the nudge window at a constant rate,
        //   hopefully good for frequency detection.

        // This didn't end up working very well, so I'm disabling it
        /*if (mod_within_window(nudge, expected_nudge_min, expected_nudge_max)) {
             flag1 = true;
             error *= m_repeat_bias;
        }*/

        // Factor 2: If a nudge is before a peak value, reduce its error. This
        // prioritizes output windows centered before large amplitudes.
        if (peak_nudge_idx < peak_nudges.size() && nudge == peak_nudges[peak_nudge_idx]) {
            flag2 = true;
            errors[1] = 0;
            peak_nudge_idx++;
        } else {
            errors[1] = 1;
        }

        double error = errors[0] * m_similarity_bias + errors[1] * m_peak_bias;

        // If we've found a nudge with lower error, use it
        if (error < min_error) {
            m_flag2 = flag2;
            min_error = error;
            best_nudge = nudge;
        }
    }

    if (other_best_nudge == best_nudge) {
        m_flag2 = false;
    }

    return best_nudge;
}

uint64_t Scope::get_total_samples() const {
    return BASS_ChannelGetLength(*m_stream_handle, BASS_POS_BYTE) / sizeof(float);
}

bool Scope::is_playing() const {
    return BASS_ChannelIsActive(*m_stream_handle) == BASS_ACTIVE_PLAYING;
}

} // namespace osmium
