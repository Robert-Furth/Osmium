#include "scope.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <queue>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif

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

// Credit to https://stackoverflow.com/a/2745086/31835764
template<typename T>
constexpr T div_ceil(T a, T b) {
    return a == 0 ? 0 : 1 + ((a - 1) / b);
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
    std::vector<float> data(static_cast<size_t>(m_samples_per_frame)
                            * m_src_num_channels);

    // Read the stream data
    uint32_t bytes_read =
        BASS_ChannelGetData(*m_stream_handle,
                            data.data(),
                            (data.size() * sizeof(float)) | BASS_DATA_FLOAT);
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

struct nudge_data {
    int32_t amount;
    int32_t dist_from_zero = 0;
    bool is_before_peak = false;
};

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
    std::vector<nudge_data> base_nudges;
    std::queue<int32_t> peaks;
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
            base_nudges.emplace_back(last_candidate_nudge);
            lower_bound_cross = false;
            upper_bound_cross = false;
        }

        if (f > peak_amp_threshold && !base_nudges.empty()
            && !base_nudges.back().is_before_peak) {
            peaks.push(offs);
            base_nudges.back().is_before_peak = true;
        }

        last_amplitude = f;
    }

    // Early exits if no candidate nudges were found
    if (base_nudges.size() == 0)
        return std::nullopt;

    // Add extra nudges in a window around the centers
    std::vector<nudge_data> nudges;
    if (m_drift_window == 0) {
        nudges = base_nudges;
    } else {
        nudges.reserve(std::max(m_drift_window, 1) * base_nudges.size());

        int32_t min_start = 0;
        for (int32_t i = 0; i < base_nudges.size(); i++) {
            const auto& base_nudge = base_nudges[i];

            // The nudge amount halfway between this nudge and the next nudge
            // If the drift window is greater than the distance between this nudge and the
            // next one, only go up to halfway so we don't insert duplicates.
            int32_t halfway = i + 1 < base_nudges.size()
                                  ? (base_nudge.amount + base_nudges[i + 1].amount) / 2
                                  : std::numeric_limits<int32_t>::max();

            int32_t start = std::max(base_nudge.amount - (m_drift_window / 2), min_start);
            int32_t end = std::min(
                {base_nudge.amount + div_ceil(m_drift_window, 2), m_max_nudge, halfway});

            for (int32_t j = start; j < end; j++) {
                bool is_before_peak = base_nudge.is_before_peak && j <= peaks.front();
                nudges.emplace_back(j, j - base_nudge.amount, is_before_peak);
            }

            if (base_nudge.is_before_peak) {
                peaks.pop();
            }

            min_start = end;
        }
    }

    // Compute an error value for each candidate nudge. This is based on
    // multiple factors that can be weighted individually by the user.
    uint32_t sim_window_start = (m_window_size - m_similarity_window) / 2;
    int32_t best_nudge = 0;
    double min_error = std::numeric_limits<double>::infinity();
    double sq_drift_window = m_drift_window * m_drift_window;

    for (const auto& nudge : nudges) {
        bool flag2 = false;

        // Factor 1: Compute the average difference between a candidate view
        // window and the previous one. (Should typically range between 0 and 2)
        double similarity_factor = 0;
        for (uint32_t i = 0; i < m_similarity_window; i++) {
            similarity_factor += std::abs(floats[nudge.amount + i + sim_window_start]
                                          - prev[i + sim_window_start]);
        }
        similarity_factor /= m_similarity_window;

        // Factor 2: If a nudge is before a peak value, reduce its error. This
        // prioritizes output windows centered before large amplitudes.
        double before_peak_factor = nudge.is_before_peak ? 0 : 1;

        // Factor 3: Penalize a nudge if it's far away from a zero
        double drift_factor =
            m_drift_window == 0
                ? 0
                : std::abs(static_cast<double>(nudge.dist_from_zero) / m_drift_window);

        double error = similarity_factor * m_similarity_bias
                       + before_peak_factor * m_peak_bias
                       + drift_factor * m_avoid_drift_bias;

        // If we've found a nudge with lower error, use it
        if (error < min_error) {
            min_error = error;
            best_nudge = nudge.amount;
        }
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
