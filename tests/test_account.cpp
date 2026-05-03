#include "test_runner.h"
#include "logic/AccountParser.h"

void ParseFullAccount() {
    std::string json = "{"
        "\"id\": \"12345\","
        "\"username\": \"alice\","
        "\"acct\": \"alice@mastodon.social\","
        "\"display_name\": \"Alice Smith\","
        "\"avatar\": \"https://example.com/avatar.png\","
        "\"note\": \"<p>Developer & coffee lover</p>\","
        "\"followers_count\": 150,"
        "\"following_count\": 80,"
        "\"statuses_count\": 500"
        "}";
    Account acct = AccountParser::ParseAccount(json);
    ASSERT_STREQ(acct.id.c_str(), "12345");
    ASSERT_STREQ(acct.username.c_str(), "alice");
    ASSERT_STREQ(acct.acct.c_str(), "alice@mastodon.social");
    ASSERT_STREQ(acct.display_name.c_str(), "Alice Smith");
    ASSERT_STREQ(acct.avatar.c_str(), "https://example.com/avatar.png");
    ASSERT_STREQ(acct.note.c_str(), "<p>Developer & coffee lover</p>");
    ASSERT_EQ(acct.followers_count, 150);
    ASSERT_EQ(acct.following_count, 80);
    ASSERT_EQ(acct.statuses_count, 500);
}

void ParsePartialAccount() {
    std::string json = "{"
        "\"id\": \"99\","
        "\"username\": \"bob\","
        "\"display_name\": \"Bob\""
        "}";
    Account acct = AccountParser::ParseAccount(json);
    ASSERT_STREQ(acct.id.c_str(), "99");
    ASSERT_STREQ(acct.username.c_str(), "bob");
    ASSERT_STREQ(acct.display_name.c_str(), "Bob");
    ASSERT_TRUE(acct.acct.empty());
    ASSERT_TRUE(acct.avatar.empty());
    ASSERT_EQ(acct.followers_count, 0);
    ASSERT_EQ(acct.following_count, 0);
    ASSERT_EQ(acct.statuses_count, 0);
}

void ParseInvalidAccountJson() {
    Account acct = AccountParser::ParseAccount("not json");
    ASSERT_TRUE(acct.id.empty());
}

void ParseEmptyObject() {
    Account acct = AccountParser::ParseAccount("{}");
    ASSERT_TRUE(acct.id.empty());
    ASSERT_EQ(acct.followers_count, 0);
}

void ParseRealisticVerifyCredentials() {
    std::string json = "{"
        "\"id\": \"110293847561234567\","
        "\"username\": \"midwan\","
        "\"acct\": \"midwan\","
        "\"display_name\": \"Dimitris\","
        "\"locked\": false,"
        "\"bot\": false,"
        "\"discoverable\": true,"
        "\"avatar\": \"https://mastodon.social/avatars/original/missing.png\","
        "\"avatar_static\": \"https://mastodon.social/avatars/original/missing.png\","
        "\"header\": \"https://mastodon.social/headers/original/missing.png\","
        "\"header_static\": \"https://mastodon.social/headers/original/missing.png\","
        "\"followers_count\": 42,"
        "\"following_count\": 133,"
        "\"statuses_count\": 789,"
        "\"note\": \"<p>Amiga enthusiast & developer</p>\","
        "\"url\": \"https://mastodon.social/@midwan\","
        "\"created_at\": \"2023-06-15T00:00:00.000Z\""
        "}";
    Account acct = AccountParser::ParseAccount(json);
    ASSERT_STREQ(acct.id.c_str(), "110293847561234567");
    ASSERT_STREQ(acct.username.c_str(), "midwan");
    ASSERT_STREQ(acct.display_name.c_str(), "Dimitris");
    ASSERT_EQ(acct.followers_count, 42);
    ASSERT_EQ(acct.following_count, 133);
    ASSERT_EQ(acct.statuses_count, 789);
    ASSERT_TRUE(acct.note.find("Amiga enthusiast") != std::string::npos);
}

void run_test_account() {
    printf("[AccountParser]\n");
    RUN_TEST(ParseFullAccount);
    RUN_TEST(ParsePartialAccount);
    RUN_TEST(ParseInvalidAccountJson);
    RUN_TEST(ParseEmptyObject);
    RUN_TEST(ParseRealisticVerifyCredentials);
}
