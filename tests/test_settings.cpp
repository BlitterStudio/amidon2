#include "test_runner.h"
#include "logic/SettingsParser.h"

void ExtractServer() {
    std::string input = "{\"server\": \"mastodon.social\"}";
    std::string result = SettingsParser::ExtractInstance(input);
    ASSERT_STREQ(result.c_str(), "mastodon.social");
}

void ExtractInstance() {
    std::string input = "{\"instance\": \"fosstodon.org\"}";
    std::string result = SettingsParser::ExtractInstance(input);
    ASSERT_STREQ(result.c_str(), "fosstodon.org");
}

void ServerTakesPriority() {
    std::string input = "{\"server\": \"mastodon.social\", \"instance\": \"other.org\"}";
    std::string result = SettingsParser::ExtractInstance(input);
    ASSERT_STREQ(result.c_str(), "mastodon.social");
}

void WhitespaceHandling() {
    std::string input = "{ \"server\"  :  \"mastodon.social\" }";
    std::string result = SettingsParser::ExtractInstance(input);
    ASSERT_STREQ(result.c_str(), "mastodon.social");
}

void EmptyInput() {
    std::string input = "{}";
    std::string result = SettingsParser::ExtractInstance(input);
    ASSERT_TRUE(result.empty());
}

void NoMatchingKey() {
    std::string input = "{\"foo\": \"bar\"}";
    std::string result = SettingsParser::ExtractInstance(input);
    ASSERT_TRUE(result.empty());
}

void PrettyPrinted() {
    std::string input = "{\n  \"server\": \"mastodon.social\"\n}\n";
    std::string result = SettingsParser::ExtractInstance(input);
    ASSERT_STREQ(result.c_str(), "mastodon.social");
}

void run_test_settings() {
    printf("[SettingsParser]\n");
    RUN_TEST(ExtractServer);
    RUN_TEST(ExtractInstance);
    RUN_TEST(ServerTakesPriority);
    RUN_TEST(WhitespaceHandling);
    RUN_TEST(EmptyInput);
    RUN_TEST(NoMatchingKey);
    RUN_TEST(PrettyPrinted);
}
