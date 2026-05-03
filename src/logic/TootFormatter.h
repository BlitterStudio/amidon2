#ifndef LOGIC_TOOT_FORMATTER_H
#define LOGIC_TOOT_FORMATTER_H

#include "MastodonTypes.h"

#include <string>

namespace TootFormatter {

std::string FormatToot(const Status& status);

}

#endif // LOGIC_TOOT_FORMATTER_H
