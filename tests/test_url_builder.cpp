#include "test_runner.h"
#include "logic/UrlBuilder.h"

void TimelineHomeUrl() {
    std::string url = UrlBuilder::TimelineHome("mastodon.social");
    ASSERT_STREQ(url.c_str(), "https://mastodon.social/api/v1/timelines/home");
}

void TimelinePublicUrl() {
    std::string url = UrlBuilder::TimelinePublic("mastodon.social");
    ASSERT_STREQ(url.c_str(), "https://mastodon.social/api/v1/timelines/public?local=true");
}

void AccountVerifyUrl() {
    std::string url = UrlBuilder::AccountVerify("fosstodon.org");
    ASSERT_STREQ(url.c_str(), "https://fosstodon.org/api/v1/accounts/verify_credentials");
}

void OAuthTokenUrl() {
    std::string url = UrlBuilder::OAuthToken("mastodon.social");
    ASSERT_STREQ(url.c_str(), "https://mastodon.social/oauth/token");
}

void RegisterAppUrl() {
    std::string url = UrlBuilder::RegisterApp("mastodon.social");
    ASSERT_STREQ(url.c_str(), "https://mastodon.social/api/v1/apps");
}

void OAuthAuthorizeUrl() {
    std::string url = UrlBuilder::OAuthAuthorize("mastodon.social", "abc123");
    ASSERT_TRUE(url.find("client_id=abc123") != std::string::npos);
    ASSERT_TRUE(url.find("response_type=code") != std::string::npos);
    ASSERT_TRUE(url.find("redirect_uri=urn:ietf:wg:oauth:2.0:oob") != std::string::npos);
    ASSERT_TRUE(url.find("scope=read+write+follow") != std::string::npos);
    ASSERT_TRUE(url.find("https://mastodon.social/oauth/authorize?") != std::string::npos);
}

void AuthHeader() {
    std::string header = UrlBuilder::AuthHeader("my-token-xyz");
    ASSERT_STREQ(header.c_str(), "Authorization: Bearer my-token-xyz");
}

void EmptyInstance() {
    std::string url = UrlBuilder::TimelineHome("");
    ASSERT_STREQ(url.c_str(), "https:///api/v1/timelines/home");
}

void run_test_url_builder() {
    printf("[UrlBuilder]\n");
    RUN_TEST(TimelineHomeUrl);
    RUN_TEST(TimelinePublicUrl);
    RUN_TEST(AccountVerifyUrl);
    RUN_TEST(OAuthTokenUrl);
    RUN_TEST(RegisterAppUrl);
    RUN_TEST(OAuthAuthorizeUrl);
    RUN_TEST(AuthHeader);
    RUN_TEST(EmptyInstance);
}
