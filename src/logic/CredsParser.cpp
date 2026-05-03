#include "MastodonTypes.h"
#include "CredsParser.h"

#include <json-c/json.h>

namespace CredsParser {

bool ParseRegistration(const std::string& json, const std::string& instance, AppRegistration& reg) {
    json_object* root = json_tokener_parse(json.c_str());
    if (!root) return false;
    if (!json_object_is_type(root, json_type_object)) {
        json_object_put(root);
        return false;
    }

    reg.instance_url = instance;
    json_object* val = NULL;
    if (json_object_object_get_ex(root, "client_id", &val) && json_object_is_type(val, json_type_string))
        reg.client_id = json_object_get_string(val);
    if (json_object_object_get_ex(root, "client_secret", &val) && json_object_is_type(val, json_type_string))
        reg.client_secret = json_object_get_string(val);

    json_object_put(root);
    return true;
}

bool ParseToken(const std::string& json, std::string& token) {
    json_object* root = json_tokener_parse(json.c_str());
    if (!root) return false;
    if (!json_object_is_type(root, json_type_object)) {
        json_object_put(root);
        return false;
    }

    json_object* val = NULL;
    if (json_object_object_get_ex(root, "access_token", &val) && json_object_is_type(val, json_type_string)) {
        token = json_object_get_string(val);
        json_object_put(root);
        return true;
    }

    json_object_put(root);
    return false;
}

bool ParseCreds(const std::string& json, const std::string& instance, AppRegistration& reg) {
    json_object* root = json_tokener_parse(json.c_str());
    if (!root) return false;
    if (!json_object_is_type(root, json_type_object)) {
        json_object_put(root);
        return false;
    }

    reg.instance_url = instance;
    json_object* val = NULL;
    if (json_object_object_get_ex(root, "client_id", &val) && json_object_is_type(val, json_type_string))
        reg.client_id = json_object_get_string(val);
    if (json_object_object_get_ex(root, "client_secret", &val) && json_object_is_type(val, json_type_string))
        reg.client_secret = json_object_get_string(val);

    json_object_put(root);
    return !reg.client_id.empty();
}

}
