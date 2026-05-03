#ifndef LOGIC_MASTODON_TYPES_H
#define LOGIC_MASTODON_TYPES_H

#include <string>

struct Status {
    std::string id;
    std::string content;
    std::string author_username;
    std::string author_displayname;
    std::string author_avatar;
    bool favourited = false;
    bool reblogged = false;
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

struct Notification {
    std::string id;
    std::string type;                 // "follow", "favourite", "reblog", "mention", etc.
    std::string author_username;
    std::string author_displayname;
    std::string author_avatar;        // URL to author's avatar image
    std::string status_excerpt;       // empty when there's no associated status
};

struct MastodonList {
    std::string id;
    std::string title;
};

#endif // LOGIC_MASTODON_TYPES_H
