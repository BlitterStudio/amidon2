#include "UrlBuilder.h"

namespace UrlBuilder {

std::string TimelineHome(const std::string& instance) {
    return "https://" + instance + "/api/v1/timelines/home";
}

std::string TimelinePublic(const std::string& instance) {
    return "https://" + instance + "/api/v1/timelines/public?local=true";
}

std::string AccountVerify(const std::string& instance) {
    return "https://" + instance + "/api/v1/accounts/verify_credentials";
}

std::string OAuthToken(const std::string& instance) {
    return "https://" + instance + "/oauth/token";
}

std::string RegisterApp(const std::string& instance) {
    return "https://" + instance + "/api/v1/apps";
}

std::string OAuthAuthorize(const std::string& instance, const std::string& clientId) {
    return "https://" + instance + "/oauth/authorize?client_id=" + clientId
           + "&response_type=code&redirect_uri=urn:ietf:wg:oauth:2.0:oob&scope=read+write+follow";
}

std::string AuthHeader(const std::string& token) {
    return "Authorization: Bearer " + token;
}

}
