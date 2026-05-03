#include "NotificationParser.h"

#include <json-c/json.h>

#include <cstdio>

namespace NotificationParser {

namespace {

void ReadAccount(json_object* acct, Notification& n) {
    if (!acct || !json_object_is_type(acct, json_type_object)) return;
    json_object* val = NULL;
    if (json_object_object_get_ex(acct, "username", &val) && json_object_is_type(val, json_type_string))
        n.author_username = json_object_get_string(val);
    if (json_object_object_get_ex(acct, "display_name", &val) && json_object_is_type(val, json_type_string))
        n.author_displayname = json_object_get_string(val);
    if (json_object_object_get_ex(acct, "avatar", &val) && json_object_is_type(val, json_type_string))
        n.author_avatar = json_object_get_string(val);
}

void ReadStatusExcerpt(json_object* status, Notification& n) {
    if (!status || !json_object_is_type(status, json_type_object)) return;
    json_object* val = NULL;
    if (json_object_object_get_ex(status, "content", &val) && json_object_is_type(val, json_type_string)) {
        n.status_excerpt = json_object_get_string(val);
    }
}

Notification ParseOne(json_object* item) {
    Notification n;
    if (!item || !json_object_is_type(item, json_type_object)) return n;

    json_object* val = NULL;
    if (json_object_object_get_ex(item, "id", &val) && json_object_is_type(val, json_type_string))
        n.id = json_object_get_string(val);
    if (json_object_object_get_ex(item, "type", &val) && json_object_is_type(val, json_type_string))
        n.type = json_object_get_string(val);
    if (json_object_object_get_ex(item, "account", &val))
        ReadAccount(val, n);
    if (json_object_object_get_ex(item, "status", &val))
        ReadStatusExcerpt(val, n);

    return n;
}

}  // namespace

std::vector<Notification> ParseNotifications(const std::string& json) {
    std::vector<Notification> out;

    json_object* root = json_tokener_parse(json.c_str());
    if (!root || !json_object_is_type(root, json_type_array)) {
        if (root) json_object_put(root);
        return out;
    }

    int len = json_object_array_length(root);
    for (int i = 0; i < len; i++) {
        json_object* item = json_object_array_get_idx(root, i);
        if (json_object_is_type(item, json_type_object)) {
            out.push_back(ParseOne(item));
        }
    }

    json_object_put(root);
    return out;
}

}  // namespace NotificationParser
