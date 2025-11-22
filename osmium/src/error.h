#ifndef ERROR_H
#define ERROR_H

#include <format>
#include <stdexcept>
#include <string>

#include <bass.h>
#include <bassmidi.h>

namespace osmium {

class Error : public std::runtime_error {
public:
    Error(const std::string& what) : std::runtime_error(what) {}
    Error(const char* what) : std::runtime_error(what) {}

    static Error from_bass_error(const std::string& prefix, int errcode) {
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
        }

        return Error(prefix + std::format("error code {}", errcode));
    }

    static Error from_bass_error(const std::string& prefix = "") {
        return Error::from_bass_error(prefix, BASS_ErrorGetCode());
    }
};

} // namespace osmium
#endif // ERROR_H
