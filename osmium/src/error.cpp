#include "error.h"

#include <format>

#include <bass.h>
#include <bassmidi.h>

namespace osmium {

Error Error::from_bass_error(const std::string& prefix, int errcode) {
    switch (errcode) {
    case BASS_ERROR_FILEOPEN:
        return Error(prefix + "could not open file");
    case BASS_ERROR_FILEFORM:
        return Error(prefix + "unsupported file format");
    case BASS_ERROR_MEM:
        return Error(prefix + "insufficient memory");
    case BASS_ERROR_MIDI_INCLUDE:
        return Error(prefix + "SFZ #include directive file could not be opened");
    case BASS_ERROR_HANDLE:
        return Error(prefix + "invalid handle");

    default:
        return Error(prefix + std::format("error code {}", errcode));
    }
}

Error Error::from_bass_error(const std::string& prefix) {
    return Error::from_bass_error(prefix, BASS_ErrorGetCode());
}

} // namespace osmium
