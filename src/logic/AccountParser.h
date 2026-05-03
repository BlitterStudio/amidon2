#ifndef LOGIC_ACCOUNT_PARSER_H
#define LOGIC_ACCOUNT_PARSER_H

#include "MastodonTypes.h"

#include <string>

namespace AccountParser {

Account ParseAccount(const std::string& json);

}

#endif // LOGIC_ACCOUNT_PARSER_H
