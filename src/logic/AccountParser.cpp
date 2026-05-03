#include "MastodonTypes.h"
#include "AccountParser.h"

#include <json-c/json.h>

namespace AccountParser {

Account ParseAccount(const std::string& json) {
    Account acct = {};

    json_object* root = json_tokener_parse(json.c_str());
    if (!root || !json_object_is_type(root, json_type_object)) {
        if (root) json_object_put(root);
        return acct;
    }

    json_object* val = NULL;
    if (json_object_object_get_ex(root, "id", &val) && json_object_is_type(val, json_type_string))
        acct.id = json_object_get_string(val);
    if (json_object_object_get_ex(root, "username", &val) && json_object_is_type(val, json_type_string))
        acct.username = json_object_get_string(val);
    if (json_object_object_get_ex(root, "acct", &val) && json_object_is_type(val, json_type_string))
        acct.acct = json_object_get_string(val);
    if (json_object_object_get_ex(root, "display_name", &val) && json_object_is_type(val, json_type_string))
        acct.display_name = json_object_get_string(val);
    if (json_object_object_get_ex(root, "avatar", &val) && json_object_is_type(val, json_type_string))
        acct.avatar = json_object_get_string(val);
    if (json_object_object_get_ex(root, "note", &val) && json_object_is_type(val, json_type_string))
        acct.note = json_object_get_string(val);
    if (json_object_object_get_ex(root, "followers_count", &val) && json_object_is_type(val, json_type_int))
        acct.followers_count = json_object_get_int(val);
    if (json_object_object_get_ex(root, "following_count", &val) && json_object_is_type(val, json_type_int))
        acct.following_count = json_object_get_int(val);
    if (json_object_object_get_ex(root, "statuses_count", &val) && json_object_is_type(val, json_type_int))
        acct.statuses_count = json_object_get_int(val);

    json_object_put(root);
    return acct;
}

}
