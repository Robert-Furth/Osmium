#include "player.h"

#include <bass.h>
#include <bassmidi.h>

#include "error.h"

namespace osmium {

Player::Player(uint32_t handle, uint32_t fps, const char* soundfont)
    : m_stream_handle(handle) {
    if (!handle)
        throw Error::from_bass_error("Error creating player: ");

    BASS_CHANNELINFO info;
    if (!BASS_ChannelGetInfo(handle, &info))
        throw Error::from_bass_error("Error getting channel info: ");

    m_num_channels = info.chans;
    m_sample_rate = info.freq;
    m_samples_per_frame = info.freq / fps;
    m_buffer.resize(static_cast<size_t>(m_samples_per_frame) * m_num_channels);

    if (soundfont) {
        HSOUNDFONT sf_handle = BASS_MIDI_FontInit(soundfont, 0);
        if (!sf_handle)
            throw Error::from_bass_error("Error initializing soundfont: ");

        m_stream_handle.set_soundfonts({sf_handle});
    }
}

Player::Player(const char* filename, uint32_t fps, const char* soundfont)
    : Player(BASS_MIDI_StreamCreateFile(BASS_FILE_NAME,
                                        filename,
                                        0,
                                        0,
                                        BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE
                                            | BASS_MIDI_DECAYEND,
                                        0),
             fps,
             soundfont) {}

bool Player::is_playing() const {
    return BASS_ChannelIsActive(*m_stream_handle) == BASS_ACTIVE_PLAYING;
}

void Player::next_wave_data() {
    std::vector<float> data(static_cast<size_t>(m_samples_per_frame) * m_num_channels);
    uint32_t bytes_read =
        BASS_ChannelGetData(*m_stream_handle,
                            data.data(),
                            data.size() * sizeof(float) | BASS_DATA_FLOAT);
    if (bytes_read == -1) {
        int errcode = BASS_ErrorGetCode();
        if (errcode != BASS_ERROR_ENDED)
            throw Error::from_bass_error("Error getting sample data: ", errcode);

        std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
        return;
    }
    uint32_t samples_read = bytes_read / sizeof(float);
    auto it = std::copy(data.cbegin(), data.cbegin() + samples_read, m_buffer.begin());
    std::fill(it, m_buffer.end(), 0.0f);
    // m_buffer.assign(data.cbegin(), data.cbegin() + samples_read);
}

} // namespace osmium
