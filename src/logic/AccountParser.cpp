#include "MastodonTypes.h"
#include "AccountParser.h"

#include <json-c/json.h>

namespace AccountParser {

namespace {

void ReadAccountFields(json_object* root, Account& acct) {
    if (!root) return;
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
}

}  // namespace

Account ParseAccount(const std::string& json) {
    Account acct = {};

    json_object* root = json_tokener_parse(json.c_str());
    if (!root || !json_object_is_type(root, json_type_object)) {
        if (root) json_object_put(root);
        return acct;
    }
    ReadAccountFields(root, acct);
    json_object_put(root);
    return acct;
}

std::vector<Account> ParseAccountArray(const std::string& json) {
    std::vector<Account> out;

    json_object* root = json_tokener_parse(json.c_str());
    if (!root || !json_object_is_type(root, json_type_array)) {
        if (root) json_object_put(root);
        return out;
    }

    int len = json_object_array_length(root);
    out.reserve(len);
    for (int i = 0; i < len; i++) {
        json_object* item = json_object_array_get_idx(root, i);
        if (json_object_is_type(item, json_type_object)) {
            Account a = {};
            ReadAccountFields(item, a);
            out.push_back(a);
        }
    }

    json_object_put(root);
    return out;
}

}
