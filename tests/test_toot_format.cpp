#include "test_runner.h"
#include "logic/TootFormatter.h"

void BasicToot() {
    Status s;
    s.author_displayname = "Alice";
    s.author_username = "alice";
    s.content = "Hello world!";
    std::string html = TootFormatter::FormatToot(s);
    ASSERT_TRUE(html.find("<html><body>") != std::string::npos);
    ASSERT_TRUE(html.find("<b>Alice</b>") != std::string::npos);
    ASSERT_TRUE(html.find("@alice") != std::string::npos);
    ASSERT_TRUE(html.find("Hello world!") != std::string::npos);
    ASSERT_TRUE(html.find("</body></html>") != std::string::npos);
}

void HtmlContent() {
    Status s;
    s.author_displayname = "Dev";
    s.author_username = "dev";
    s.content = "<p>Post with <a href=\"http://example.com\">link</a></p>";
    std::string html = TootFormatter::FormatToot(s);
    ASSERT_TRUE(html.find("<a href") != std::string::npos);
}

void EmptyContent() {
    Status s;
    s.author_displayname = "User";
    s.author_username = "user";
    s.content = "";
    std::string html = TootFormatter::FormatToot(s);
    ASSERT_TRUE(html.find("<b>User</b>") != std::string::npos);
    ASSERT_TRUE(html.find("<i>@user</i>") != std::string::npos);
}

void EmptyNames() {
    Status s;
    s.author_displayname = "";
    s.author_username = "";
    s.content = "Just a post";
    std::string html = TootFormatter::FormatToot(s);
    ASSERT_TRUE(html.find("Just a post") != std::string::npos);
}

void Structure() {
    Status s;
    s.author_displayname = "T";
    s.author_username = "t";
    s.content = "X";
    std::string html = TootFormatter::FormatToot(s);
    ASSERT_STREQ(html.c_str(),
        "<html><body><b>T</b> <i>@t</i><br><br>X</body></html>");
}

void run_test_toot_format() {
    printf("[TootFormatter]\n");
    RUN_TEST(BasicToot);
    RUN_TEST(HtmlContent);
    RUN_TEST(EmptyContent);
    RUN_TEST(EmptyNames);
    RUN_TEST(Structure);
}
