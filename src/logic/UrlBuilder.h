#ifndef LOGIC_URL_BUILDER_H
#define LOGIC_URL_BUILDER_H

#include <string>

namespace UrlBuilder {

std::string TimelineHome(const std::string& instance);
std::string TimelinePublic(const std::string& instance);
std::string Notifications(const std::string& instance);
std::string Favourites(const std::string& instance);
std::string Bookmarks(const std::string& instance);
std::string TrendingStatuses(const std::string& instance);
std::string Conversations(const std::string& instance);
std::string Lists(const std::string& instance);
std::string FollowRequests(const std::string& instance);
std::string FavouriteStatus(const std::string& instance, const std::string& statusId);
std::string UnfavouriteStatus(const std::string& instance, const std::string& statusId);
std::string ReblogStatus(const std::string& instance, const std::string& statusId);
std::string UnreblogStatus(const std::string& instance, const std::string& statusId);
std::string AccountVerify(const std::string& instance);
std::string OAuthToken(const std::string& instance);
std::string RegisterApp(const std::string& instance);
std::string OAuthAuthorize(const std::string& instance, const std::string& clientId);
std::string AuthHeader(const std::string& token);

}

#endif // LOGIC_URL_BUILDER_H
