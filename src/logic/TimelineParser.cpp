#include "MastodonTypes.h"
#include "TimelineParser.h"

#include <json-c/json.h>

#include <cstdio>

namespace TimelineParser {

Status ParseStatus(json_object* item) {
    Status s;
    if (!item || !json_object_is_type(item, json_type_object))
        return s;

    json_object* val = NULL;
    if (json_object_object_get_ex(item, "id", &val) && json_object_is_type(val, json_type_string))
        s.id = json_object_get_string(val);
    if (json_object_object_get_ex(item, "content", &val) && json_object_is_type(val, json_type_string))
        s.content = json_object_get_string(val);
    if (json_object_object_get_ex(item, "favourited", &val) && json_object_is_type(val, json_type_boolean))
        s.favourited = json_object_get_boolean(val);
    if (json_object_object_get_ex(item, "reblogged", &val) && json_object_is_type(val, json_type_boolean))
        s.reblogged = json_object_get_boolean(val);
    if (json_object_object_get_ex(item, "account", &val) && json_object_is_type(val, json_type_object)) {
        json_object* acctVal = NULL;
        if (json_object_object_get_ex(val, "username", &acctVal) && json_object_is_type(acctVal, json_type_string))
            s.author_username = json_object_get_string(acctVal);
        if (json_object_object_get_ex(val, "display_name", &acctVal) && json_object_is_type(acctVal, json_type_string))
            s.author_displayname = json_object_get_string(acctVal);
    }
    return s;
}

std::vector<Status> ParseTimeline(const std::string& json) {
    std::vector<Status> timeline;

    json_object* root = json_tokener_parse(json.c_str());
    if (!root || !json_object_is_type(root, json_type_array)) {
        if (root) json_object_put(root);
        return timeline;
    }

    int len = json_object_array_length(root);
    for (int i = 0; i < len; i++) {
        json_object* item = json_object_array_get_idx(root, i);
        if (json_object_is_type(item, json_type_object)) {
            timeline.push_back(ParseStatus(item));
        }
    }

    json_object_put(root);
    return timeline;
}

}
