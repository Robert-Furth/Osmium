#ifndef ERROR_H
#define ERROR_H

#include <stdexcept>
#include <string>

namespace osmium {

class Error : public std::runtime_error {
public:
    Error(const std::string& what) : std::runtime_error(what) {}
    Error(const char* what) : std::runtime_error(what) {}

    static Error from_bass_error(const std::string& prefix, int errcode);
    static Error from_bass_error(const std::string& prefix = "");
};

} // namespace osmium
#endif // ERROR_H
