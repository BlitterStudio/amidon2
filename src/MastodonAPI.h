/*
 * Amidon - a Mastodon client for AmigaOS
 * Copyright (C) 2024 Dimitris Panokostas
 */

#include "HttpClient.h"

#ifndef MASTODON_API_H
#define MASTODON_API_H

#include <string>
#include <vector>
#include <functional>
#include "picojson.h"

// Picojson used for parsing


struct Status {
    std::string id;
    std::string content;
    std::string author_username;
    std::string author_displayname;
};

struct AppRegistration {
    std::string client_id;
    std::string client_secret;
    std::string instance_url; // helper
};

class MastodonAPI {
public:
    MastodonAPI();
    
    // Auth Flow
    // 1. Register App -> returns client_id, client_secret
    void RegisterApp(const std::string& instance, std::function<void(bool success, const AppRegistration& reg)> callback);
    
    // 2. Get Token -> exchanges auth code for access token
    void GetToken(const AppRegistration& reg, const std::string& authCode, std::function<void(bool success, const std::string& token)> callback);
    
    // 3. Set Credentials (if loaded from disk)
    void SetCredentials(const std::string& instance, const std::string& token);

    // Fetch Data
    void GetHomeTimeline(std::function<void(std::vector<Status>)> callback);
    void GetPublicTimeline(std::function<void(std::vector<Status>)> callback);
    void GetNotifications(std::function<void(std::vector<std::string>)> callback);

    bool HasCredentials() const;
    
    // Deprecated/Refactored
    bool Login(const std::string& instance, const std::string& email, const std::string& password);

private:
    std::string m_InstanceURL;
    std::string m_AccessToken;
    HttpClient m_Client;
};

#endif // MASTODON_API_H
