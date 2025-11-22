#ifndef SCOPEBUILDER_H
#define SCOPEBUILDER_H

#include <string>
#include <vector>

#include "scope.h"

#define BUILDER_DEF(_type, _name, _var_name, _default) \
private: \
    _type m_##_name = _default; \
\
public: \
    ScopeBuilder& _name(_type _var_name) { \
        m_##_name = _var_name; \
        return *this; \
    }

namespace osmium {

class ScopeBuilder {
    BUILDER_DEF(unsigned, frame_rate, fps, 30)
    // BUILDER_DEF(unsigned, sample_rate, hz, 48000)
    BUILDER_DEF(bool, stereo, stereo, true)
    BUILDER_DEF(double, trigger_threshold, threshold, 0.1)
    BUILDER_DEF(double, amplification, amplification, 1.0);
    BUILDER_DEF(unsigned, max_nudge_ms, ms, 5);
    BUILDER_DEF(unsigned, display_window_ms, ms, 40);
    BUILDER_DEF(unsigned, similarity_window_ms, ms, 40);
    BUILDER_DEF(double, similarity_bias, weight, 1.0);

    // BUILDER_DEF(double, repeat_bias, factor, 0.75)
    // BUILDER_DEF(unsigned, repeat_bias_window_ms, ms, 3)
    BUILDER_DEF(double, peak_bias, weight, 0.5)
    BUILDER_DEF(double, peak_threshold, factor, 0.9)

public:
    ScopeBuilder& soundfonts(const std::vector<std::string>& soundfonts) {
        m_soundfonts = soundfonts;
        return *this;
    }

    Scope build_from_file(const char* filename);
    Scope build_from_midi_channel(const char* filename, int channel);
    Scope build_from_handle(unsigned long handle);

private:
    std::vector<std::string> m_soundfonts;
};

} // namespace osmium

#undef BUILDER_DEF

#endif // SCOPEBUILDER_H
