#ifndef OSMIUM_H
#define OSMIUM_H

#include "error.h"        // IWYU pragma: export
#include "eventtracker.h" // IWYU pragma: export
#include "player.h"       // IWYU pragma: export
#include "scope.h"        // IWYU pragma: export
#include "scopebuilder.h" // IWYU pragma: export

namespace osmium {

bool init();
void uninit();

} // namespace osmium

#endif // OSMIUM_H
