#include "test_runner.h"
#include "logic/TimelineParser.h"

void ParseSingleStatus() {
    std::string json = "[{\"id\": \"123\", \"content\": \"Hello world\", "
                       "\"account\": {\"username\": \"alice\", \"display_name\": \"Alice\"}}]";
    std::vector<Status> result = TimelineParser::ParseTimeline(json);
    ASSERT_EQ(result.size(), 1u);
    ASSERT_STREQ(result[0].id.c_str(), "123");
    ASSERT_STREQ(result[0].content.c_str(), "Hello world");
    ASSERT_STREQ(result[0].author_username.c_str(), "alice");
    ASSERT_STREQ(result[0].author_displayname.c_str(), "Alice");
}

void ParseMultipleStatuses() {
    std::string json = "["
        "{\"id\": \"1\", \"content\": \"First\", \"account\": {\"username\": \"a\", \"display_name\": \"A\"}},"
        "{\"id\": \"2\", \"content\": \"Second\", \"account\": {\"username\": \"b\", \"display_name\": \"B\"}},"
        "{\"id\": \"3\", \"content\": \"Third\", \"account\": {\"username\": \"c\", \"display_name\": \"C\"}}"
        "]";
    std::vector<Status> result = TimelineParser::ParseTimeline(json);
    ASSERT_EQ(result.size(), 3u);
    ASSERT_STREQ(result[0].id.c_str(), "1");
    ASSERT_STREQ(result[1].id.c_str(), "2");
    ASSERT_STREQ(result[2].id.c_str(), "3");
}

void ParseEmptyArray() {
    std::string json = "[]";
    std::vector<Status> result = TimelineParser::ParseTimeline(json);
    ASSERT_EQ(result.size(), 0u);
}

void ParseInvalidJson() {
    std::string json = "not json";
    std::vector<Status> result = TimelineParser::ParseTimeline(json);
    ASSERT_EQ(result.size(), 0u);
}

void ParseNonArrayJson() {
    std::string json = "{\"id\": \"123\"}";
    std::vector<Status> result = TimelineParser::ParseTimeline(json);
    ASSERT_EQ(result.size(), 0u);
}

void ParseStatusWithHtmlContent() {
    std::string json = "[{\"id\": \"999\", \"content\": \"<p>Hello <a href=\\\"http://example.com\\\">link</a></p>\", "
                       "\"account\": {\"username\": \"user\", \"display_name\": \"Test User\"}}]";
    std::vector<Status> result = TimelineParser::ParseTimeline(json);
    ASSERT_EQ(result.size(), 1u);
    ASSERT_TRUE(result[0].content.find("<p>Hello") != std::string::npos);
    ASSERT_TRUE(result[0].content.find("<a href") != std::string::npos);
}

void ParseStatusMissingFields() {
    std::string json = "[{\"id\": \"42\"}]";
    std::vector<Status> result = TimelineParser::ParseTimeline(json);
    ASSERT_EQ(result.size(), 1u);
    ASSERT_STREQ(result[0].id.c_str(), "42");
    ASSERT_TRUE(result[0].content.empty());
    ASSERT_TRUE(result[0].author_username.empty());
}

void ParseRealisticMastodonTimeline() {
    std::string json = "[{"
        "\"id\": \"111600543832764635\","
        "\"created_at\": \"2024-01-15T10:30:00.000Z\","
        "\"content\": \"<p>Just shipped a new feature! 🚀</p>\","
        "\"visibility\": \"public\","
        "\"account\": {"
            "\"id\": \"12345\","
            "\"username\": \"devuser\","
            "\"display_name\": \"Dev User\","
            "\"acct\": \"devuser@mastodon.social\","
            "\"avatar\": \"https://example.com/avatar.png\""
        "}"
    "}]";
    std::vector<Status> result = TimelineParser::ParseTimeline(json);
    ASSERT_EQ(result.size(), 1u);
    ASSERT_STREQ(result[0].id.c_str(), "111600543832764635");
    ASSERT_STREQ(result[0].author_username.c_str(), "devuser");
    ASSERT_STREQ(result[0].author_displayname.c_str(), "Dev User");
}

void run_test_timeline() {
    printf("[TimelineParser]\n");
    RUN_TEST(ParseSingleStatus);
    RUN_TEST(ParseMultipleStatuses);
    RUN_TEST(ParseEmptyArray);
    RUN_TEST(ParseInvalidJson);
    RUN_TEST(ParseNonArrayJson);
    RUN_TEST(ParseStatusWithHtmlContent);
    RUN_TEST(ParseStatusMissingFields);
    RUN_TEST(ParseRealisticMastodonTimeline);
}
