#ifndef LOGIC_URL_BUILDER_H
#define LOGIC_URL_BUILDER_H

#include <string>

namespace UrlBuilder {

std::string TimelineHome(const std::string& instance);
std::string TimelinePublic(const std::string& instance);
std::string AccountVerify(const std::string& instance);
std::string OAuthToken(const std::string& instance);
std::string RegisterApp(const std::string& instance);
std::string OAuthAuthorize(const std::string& instance, const std::string& clientId);
std::string AuthHeader(const std::string& token);

}

#endif // LOGIC_URL_BUILDER_H
