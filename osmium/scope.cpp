#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "scope.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

#include <bass.h>

#include "error.h"

namespace osmium {

// --- helper functions ---

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

// --- osmium::Scope implementation ---

Scope::Scope(uint32_t handle,
             uint32_t samples_per_frame,
             uint32_t sample_rate,
             uint32_t window_size,
             uint32_t max_nudge,
             float amplification,
             float trigger_threshold,
             unsigned src_num_channels,
             bool is_stereo)
    : m_stream_handle(handle),
      m_samples_per_frame(samples_per_frame),
      m_sample_rate(sample_rate),
      m_window_size(window_size),
      m_max_nudge(max_nudge),
      m_trigger_threshold(trigger_threshold),
      m_amplification(amplification),
      m_src_num_channels(src_num_channels),
      m_is_stereo(is_stereo) {
    m_left_output.resize(m_window_size);
    m_right_output.resize(m_window_size);
    m_left_buffer.resize(m_window_size + 2 * max_nudge);
    m_right_buffer.resize(m_window_size + 2 * max_nudge);
}

void Scope::next_wave_data() {
    update_buffers();

    auto maybe_startpoint = find_best_startpoint(m_left_buffer, m_left_output);
    if (!maybe_startpoint.has_value() && m_is_stereo) {
        maybe_startpoint = find_best_startpoint(m_right_buffer, m_right_output);
    }

    size_t startpoint = maybe_startpoint.value_or(m_max_nudge);
    m_left_output.assign(m_left_buffer.cbegin() + startpoint,
                         m_left_buffer.cbegin() + startpoint + m_window_size);
    if (m_is_stereo) {
        m_right_output.assign(m_right_buffer.cbegin() + startpoint,
                              m_right_buffer.cbegin() + startpoint + m_window_size);
    }
}

void Scope::update_buffers() {
    std::vector<float> data(m_samples_per_frame * m_src_num_channels);

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
    m_total_samples_read += samples_read * m_src_num_channels;

    // Demux the data into left and right channels
    std::vector<float> new_left_samples(samples_read);
    std::vector<float> new_right_samples(samples_read);

    if (m_is_stereo) {
        if (m_src_num_channels == 1) {
            for (uint32_t i = 0; i < samples_read; i++) {
                new_left_samples[i] = data[i] * m_amplification;
                new_right_samples[i] = data[i] * m_amplification;
            }
        } else {
            // TODO: Maybe try downmixing 5.1/7.1 to stereo? Probably overkill tbh.
            for (uint32_t i = 0; i < samples_read; i++) {
                uint32_t j = i * m_src_num_channels;
                new_left_samples[i] = data[j] * m_amplification;
                new_right_samples[i] = data[j + 1] * m_amplification;
            }
        }
    } else {
        if (m_src_num_channels == 1) {
            for (uint32_t i = 0; i < samples_read; i++) {
                new_left_samples[i] = data[i] * m_amplification;
            }
        } else {
            for (uint32_t i = 0; i < samples_read; i++) {
                uint32_t j = i * m_src_num_channels;
                new_left_samples[i] = (data[j] + data[j + 1]) * m_amplification * 0.5;
                // new_left_samples[i] = data[i] * m_amplification;
            }
        }
    }

    // Shift the samples into their respective buffers.
    shift_in(m_left_buffer, new_left_samples, m_samples_per_frame);
    if (m_is_stereo) {
        shift_in(m_right_buffer, new_right_samples, m_samples_per_frame);
    }
}

std::optional<size_t> Scope::find_best_startpoint(const std::vector<float>& floats,
                                                  const std::vector<float>& prev) {
    // floats.size() == m_window_size + 2 * m_max_nudge
    size_t midpoint = floats.size() / 2;
    size_t window_start = midpoint;

    float max_in_window = *std::max_element(floats.cbegin() + window_start,
                                            floats.cbegin() + window_start + m_max_nudge);

    // Find all rising edges within the window
    float lower_bound = 0.0 * max_in_window;
    float upper_bound = m_trigger_threshold * max_in_window;

    bool flag = 0;
    float last_f = 0.0;
    int64_t last_zero_cross = 0;
    std::vector<int64_t> nudges;
    // int64_t minimum_nudge_found = std::numeric_limits<int64_t>::max();

    for (int64_t offs = 0; offs < m_max_nudge; offs++) {
        float f = floats[window_start + offs];
        if (last_f < 0 && f >= 0) {
            last_zero_cross = offs;
        }

        if (f < lower_bound) {
            flag = false;
        }
        if (last_f < lower_bound && f >= lower_bound) {
            flag = true;
        }
        if (flag && last_f < upper_bound && f >= upper_bound) {
            nudges.push_back(last_zero_cross);
            // if (std::abs(last_zero_cross) < std::abs(minimum_nudge_found))
            //     minimum_nudge_found = last_zero_cross;
            flag = false;
        }

        last_f = f;
    }

    if (nudges.size() == 0) {
        m_nudge_amount = 0;
        return std::optional<size_t>();
    }
    if (nudges.size() == 1) {
        m_nudge_amount = nudges[0];
        return nudges[0] + m_max_nudge;
    }

    // Out of all candidate startpoints, find the one with the minimum difference to the previous one
    size_t candidate_start = 0;
    double min_error = std::numeric_limits<double>::infinity();
    for (int64_t nudge : nudges) {
        size_t start = m_max_nudge + nudge;
        double error = 0;
        for (size_t i = 0; i < m_window_size; i++) {
            error += std::abs(floats[start + i] - prev[i]) /* / max_in_window*/;
        }

        if (error < min_error) {
            candidate_start = start;
            min_error = error;
        }
    }

    m_nudge_amount = candidate_start - m_max_nudge;

    return candidate_start;
}

uint64_t Scope::get_total_samples() const {
    return BASS_ChannelGetLength(*m_stream_handle, BASS_POS_BYTE) / sizeof(float);
}

bool Scope::is_playing() const {
    return BASS_ChannelIsActive(*m_stream_handle) == BASS_ACTIVE_PLAYING;
}

} // namespace osmium
