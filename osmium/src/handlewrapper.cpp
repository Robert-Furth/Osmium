#include "handlewrapper.h"

#include <bass.h>
#include <bassmidi.h>

#include "error.h"

namespace osmium {

HandleWrapper::HandleWrapper(uint32_t handle) : m_handle(handle), m_extra_data(nullptr) {}

HandleWrapper::HandleWrapper(HandleWrapper&& other) noexcept : m_handle(other.m_handle) {
    other.m_handle = 0;
    m_extra_data.swap(other.m_extra_data);
    m_soundfont_handles = std::move(other.m_soundfont_handles);
    other.m_soundfont_handles.clear();
}

HandleWrapper& HandleWrapper::operator=(HandleWrapper&& other) noexcept {
    m_handle = other.m_handle;
    other.m_handle = 0;
    m_extra_data.swap(other.m_extra_data);
    m_soundfont_handles = std::move(other.m_soundfont_handles);
    other.m_soundfont_handles.clear();
    return *this;
}

HandleWrapper::~HandleWrapper() {
    if (m_handle) {
        BASS_StreamFree(m_handle);
        m_handle = 0;
    }

    for (HSOUNDFONT hsf : m_soundfont_handles) {
        BASS_MIDI_FontFree(hsf);
    }
}

uint32_t HandleWrapper::operator*() const {
    return m_handle;
}

void HandleWrapper::set_extra_data(int n) {
    m_extra_data = std::make_unique<int>(n);
}

const std::unique_ptr<int>& HandleWrapper::extra_data_ptr() {
    return m_extra_data;
}

void HandleWrapper::set_soundfonts(const std::vector<HSOUNDFONT>& soundfonts) {
    m_soundfont_handles = soundfonts;

    std::vector<BASS_MIDI_FONT> font_structs;
    font_structs.reserve(soundfonts.size());
    for (auto hsf : soundfonts) {
        font_structs.emplace_back(BASS_MIDI_FONT{hsf, -1, 0});
    }

    if (!BASS_MIDI_StreamSetFonts(m_handle, font_structs.data(), font_structs.size()))
        throw Error::from_bass_error("Error applying soundfonts: ");
}

} // namespace osmium
