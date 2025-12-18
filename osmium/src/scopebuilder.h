#ifndef SCOPEBUILDER_H
#define SCOPEBUILDER_H

#include <string>
#include <vector>

#include "error.h"
#include "scope.h"

#define BUILDER_DEF_CHECK(_type, _name, _var_name, _default, _check)                     \
private:                                                                                 \
    _type m_##_name = _default;                                                          \
                                                                                         \
public:                                                                                  \
    ScopeBuilder& _name(const _type&(_var_name)) {                                       \
        _check;                                                                          \
        m_##_name = _var_name;                                                           \
        return *this;                                                                    \
    }

#define BUILDER_DEF(_type, _name, _var_name, _default)                                   \
    BUILDER_DEF_CHECK(_type, _name, _var_name, _default, )

#define BUILDER_DEF_NONNEG(_type, _name, _var_name, _default)                            \
    BUILDER_DEF_CHECK(_type,                                                             \
                      _name,                                                             \
                      _var_name,                                                         \
                      _default,                                                          \
                      if ((_var_name) < 0) throw Error(#_name " cannot be negative");)

namespace osmium {

class ScopeBuilder {
    BUILDER_DEF_NONNEG(int, frame_rate, fps, 30)
    // BUILDER_DEF(unsigned, sample_rate, hz, 48000)
    BUILDER_DEF(bool, stereo, stereo, true)
    BUILDER_DEF(double, trigger_threshold, threshold, 0.1)
    BUILDER_DEF(double, amplification, amplification, 1.0);
    BUILDER_DEF_NONNEG(int, max_nudge_ms, ms, 40);
    BUILDER_DEF_NONNEG(int, display_window_ms, ms, 40);

    BUILDER_DEF_NONNEG(int, similarity_window_ms, ms, 40);
    BUILDER_DEF(double, similarity_bias, weight, 1.0);
    BUILDER_DEF(double, peak_threshold, factor, 0.9)
    BUILDER_DEF(double, peak_bias, weight, 0.5)
    BUILDER_DEF_NONNEG(double, drift_window, ms, 0.0)
    BUILDER_DEF(double, avoid_drift_bias, weight, 1.0)

    // NOLINTNEXTLINE(readability-redundant-member-init)
    BUILDER_DEF(std::vector<std::string>, soundfonts, soundfonts, {})

public:
    Scope build_from_file(const char* filename);
    Scope build_from_midi_channel(const char* filename, int channel);
    Scope build_from_handle(unsigned long handle);
};

} // namespace osmium

#undef BUILDER_DEF_CHECK
#undef BUILDER_DEF
#undef BUILDER_DEF_NONNEG

#endif // SCOPEBUILDER_H
