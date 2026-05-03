#include "UrlBuilder.h"

namespace UrlBuilder {

std::string TimelineHome(const std::string& instance) {
    return "https://" + instance + "/api/v1/timelines/home";
}

std::string TimelinePublic(const std::string& instance) {
    return "https://" + instance + "/api/v1/timelines/public?local=true";
}

std::string Notifications(const std::string& instance) {
    return "https://" + instance + "/api/v1/notifications";
}

std::string Favourites(const std::string& instance) {
    return "https://" + instance + "/api/v1/favourites";
}

std::string Bookmarks(const std::string& instance) {
    return "https://" + instance + "/api/v1/bookmarks";
}

std::string TrendingStatuses(const std::string& instance) {
    return "https://" + instance + "/api/v1/trends/statuses";
}

std::string Conversations(const std::string& instance) {
    return "https://" + instance + "/api/v1/conversations";
}

std::string Lists(const std::string& instance) {
    return "https://" + instance + "/api/v1/lists";
}

std::string FollowRequests(const std::string& instance) {
    return "https://" + instance + "/api/v1/follow_requests";
}

std::string FavouriteStatus(const std::string& instance, const std::string& statusId) {
    return "https://" + instance + "/api/v1/statuses/" + statusId + "/favourite";
}

std::string UnfavouriteStatus(const std::string& instance, const std::string& statusId) {
    return "https://" + instance + "/api/v1/statuses/" + statusId + "/unfavourite";
}

std::string ReblogStatus(const std::string& instance, const std::string& statusId) {
    return "https://" + instance + "/api/v1/statuses/" + statusId + "/reblog";
}

std::string UnreblogStatus(const std::string& instance, const std::string& statusId) {
    return "https://" + instance + "/api/v1/statuses/" + statusId + "/unreblog";
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
