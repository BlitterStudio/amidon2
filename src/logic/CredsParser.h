#ifndef LOGIC_CREDS_PARSER_H
#define LOGIC_CREDS_PARSER_H

#include "MastodonTypes.h"

#include <string>

namespace CredsParser {

bool ParseRegistration(const std::string& json, const std::string& instance, AppRegistration& reg);
bool ParseToken(const std::string& json, std::string& token);
bool ParseCreds(const std::string& json, const std::string& instance, AppRegistration& reg);

}

#endif // LOGIC_CREDS_PARSER_H
