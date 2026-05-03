#ifndef LOGIC_MASTODON_TYPES_H
#define LOGIC_MASTODON_TYPES_H

#include <string>

struct Status {
    std::string id;
    std::string content;
    std::string author_username;
    std::string author_displayname;
};

struct AppRegistration {
    std::string client_id;
    std::string client_secret;
    std::string instance_url;
};

struct Account {
    std::string id;
    std::string username;
    std::string acct;
    std::string display_name;
    std::string avatar;
    std::string note;
    int followers_count;
    int following_count;
    int statuses_count;
};

#endif // LOGIC_MASTODON_TYPES_H
