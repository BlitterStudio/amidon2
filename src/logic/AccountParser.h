#ifndef LOGIC_ACCOUNT_PARSER_H
#define LOGIC_ACCOUNT_PARSER_H

#include "MastodonTypes.h"

#include <string>
#include <vector>

namespace AccountParser {

Account ParseAccount(const std::string& json);
std::vector<Account> ParseAccountArray(const std::string& json);

}

#endif // LOGIC_ACCOUNT_PARSER_H
