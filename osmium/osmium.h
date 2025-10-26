#ifndef OSMIUM_H
#define OSMIUM_H

#include "error.h"        // IWYU pragma: export
#include "player.h"       // IWYU pragma: export
#include "scope.h"        // IWYU pragma: export
#include "scopebuilder.h" // IWYU pragma: export

namespace osmium {

bool init();
void uninit();

// bool set_soundfonts(const std::vector<std::string>& soundfonts);

} // namespace osmium

#endif // OSMIUM_H
