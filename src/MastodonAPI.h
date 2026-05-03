/*
 * Amidon2 - a Mastodon client for AmigaOS
 * Copyright (C) 2026 Dimitris Panokostas
 */

#ifndef MASTODON_API_H
#define MASTODON_API_H

#include "AsyncHttpService.h"
#include "logic/MastodonTypes.h"

#include <string>
#include <vector>
#include <functional>

#include <json-c/json.h>

class MastodonAPI {
public:
    MastodonAPI();

    void SetAsyncHttp(AsyncHttpService* asyncHttp);

    void RegisterApp(const std::string& instance, std::function<void(bool success, const AppRegistration& reg)> callback);
    void GetToken(const AppRegistration& reg, const std::string& authCode, std::function<void(bool success, const std::string& token)> callback);
    void SetCredentials(const std::string& instance, const std::string& token);

    void GetHomeTimeline(std::function<void(std::vector<Status>)> callback);
    void GetPublicTimeline(std::function<void(std::vector<Status>)> callback);
    void GetNotifications(std::function<void(std::vector<std::string>)> callback);

    void PostStatus(const std::string& content, const std::string& visibility, const std::string& spoilerText, std::function<void(bool success, const std::string& statusId)> callback);

    void GetAccountInfo(std::function<void(bool success, const Account& account)> callback);

    bool HasCredentials() const;

    bool Login(const std::string& instance, const std::string& email, const std::string& password);

private:
    std::string m_InstanceURL;
    std::string m_AccessToken;
    AsyncHttpService* m_AsyncHttp;
};

#endif // MASTODON_API_H
