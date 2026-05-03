#include "test_runner.h"
#include "logic/CredsParser.h"

void ParseValidRegistration() {
    std::string json = "{"
        "\"id\": \"1\","
        "\"name\": \"Amidon\","
        "\"redirect_uri\": \"urn:ietf:wg:oauth:2.0:oob\","
        "\"client_id\": \"test_client_id\","
        "\"client_secret\": \"test_client_secret\""
        "}";
    AppRegistration reg;
    bool ok = CredsParser::ParseRegistration(json, "mastodon.social", reg);
    ASSERT_TRUE(ok);
    ASSERT_STREQ(reg.instance_url.c_str(), "mastodon.social");
    ASSERT_STREQ(reg.client_id.c_str(), "test_client_id");
    ASSERT_STREQ(reg.client_secret.c_str(), "test_client_secret");
}

void ParseInvalidRegistrationJson() {
    AppRegistration reg;
    bool ok = CredsParser::ParseRegistration("not json", "mastodon.social", reg);
    ASSERT_FALSE(ok);
}

void ParseTokenValid() {
    std::string json = "{"
        "\"access_token\": \"abc123token\","
        "\"token_type\": \"Bearer\","
        "\"scope\": \"read write follow push\","
        "\"created_at\": 1700000000"
        "}";
    std::string token;
    bool ok = CredsParser::ParseToken(json, token);
    ASSERT_TRUE(ok);
    ASSERT_STREQ(token.c_str(), "abc123token");
}

void ParseTokenMissing() {
    std::string json = "{\"token_type\": \"Bearer\"}";
    std::string token;
    bool ok = CredsParser::ParseToken(json, token);
    ASSERT_FALSE(ok);
}

void ParseTokenInvalidJson() {
    std::string token;
    bool ok = CredsParser::ParseToken("not json", token);
    ASSERT_FALSE(ok);
}

void ParseCredsValid() {
    std::string json = "{"
        "\"client_id\": \"my_id\","
        "\"client_secret\": \"my_secret\""
        "}";
    AppRegistration reg;
    bool ok = CredsParser::ParseCreds(json, "fosstodon.org", reg);
    ASSERT_TRUE(ok);
    ASSERT_STREQ(reg.instance_url.c_str(), "fosstodon.org");
    ASSERT_STREQ(reg.client_id.c_str(), "my_id");
    ASSERT_STREQ(reg.client_secret.c_str(), "my_secret");
}

void ParseCredsEmpty() {
    std::string json = "{}";
    AppRegistration reg;
    bool ok = CredsParser::ParseCreds(json, "mastodon.social", reg);
    ASSERT_FALSE(ok);
}

void ParseCredsInvalidJson() {
    AppRegistration reg;
    bool ok = CredsParser::ParseCreds("bad", "mastodon.social", reg);
    ASSERT_FALSE(ok);
}

void run_test_creds() {
    printf("[CredsParser]\n");
    RUN_TEST(ParseValidRegistration);
    RUN_TEST(ParseInvalidRegistrationJson);
    RUN_TEST(ParseTokenValid);
    RUN_TEST(ParseTokenMissing);
    RUN_TEST(ParseTokenInvalidJson);
    RUN_TEST(ParseCredsValid);
    RUN_TEST(ParseCredsEmpty);
    RUN_TEST(ParseCredsInvalidJson);
}
