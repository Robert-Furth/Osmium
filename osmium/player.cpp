#include "player.h"

#include <bass.h>
#include <bassmidi.h>

#include "../constants.h"

namespace osmium {

Player::Player(uint32_t handle, uint32_t fps) : m_stream_handle(handle) {
    BASS_CHANNELINFO info;
    BASS_ChannelGetInfo(handle, &info);

    m_num_channels = info.chans;
    m_sample_rate = info.freq;
    m_samples_per_frame = info.freq / fps;
    m_buffer_full.resize(m_samples_per_frame * info.chans);

    // TODO Don't hardcode soundfont
    HSOUNDFONT sf_handle = BASS_MIDI_FontInit(DEBUG_SOUNDFONT_PATH, 0);
    if (!sf_handle) {
        // TODO error handling
        // throw std::runtime_error("Could not initialize soundfont");
    }

    m_stream_handle.set_soundfonts({sf_handle});
}

Player::Player(const char* filename, uint32_t fps)
    : Player(BASS_StreamCreateFile(BASS_FILE_NAME,
                                   filename,
                                   0,
                                   0,
                                   BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE
                                       | BASS_MIDI_DECAYEND),
             fps) {}

bool Player::is_playing() const {
    return BASS_ChannelIsActive(*m_stream_handle) == BASS_ACTIVE_PLAYING;
}

void Player::next_wave_data() {
    uint32_t bytes_read = BASS_ChannelGetData(*m_stream_handle,
                                              m_buffer_full.data(),
                                              m_buffer_full.size() * sizeof(float)
                                                  | BASS_DATA_FLOAT);
    uint32_t samples_read = bytes_read / sizeof(float);
    m_buffer.assign(m_buffer_full.cbegin(), m_buffer_full.cbegin() + samples_read);
}

} // namespace osmium
