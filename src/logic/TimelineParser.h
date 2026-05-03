#ifndef LOGIC_TIMELINE_PARSER_H
#define LOGIC_TIMELINE_PARSER_H

#include "MastodonTypes.h"

#include <json-c/json.h>
#include <string>
#include <vector>

namespace TimelineParser {

Status ParseStatus(json_object* item);
std::vector<Status> ParseTimeline(const std::string& json);

}

#endif // LOGIC_TIMELINE_PARSER_H
