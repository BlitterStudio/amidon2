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
    void GetFavourites(std::function<void(std::vector<Status>)> callback);
    void GetBookmarks(std::function<void(std::vector<Status>)> callback);
    void GetTrendingStatuses(std::function<void(std::vector<Status>)> callback);
    void GetConversations(std::function<void(std::vector<Status>)> callback);
    void GetLists(std::function<void(std::vector<MastodonList>)> callback);
    void GetFollowRequests(std::function<void(std::vector<Account>)> callback);
    void GetNotifications(std::function<void(std::vector<Notification>)> callback);

    void PostStatus(const std::string& content, const std::string& visibility, const std::string& spoilerText, std::function<void(bool success, const std::string& statusId)> callback);

    void GetAccountInfo(std::function<void(bool success, const Account& account)> callback);

    // Toot interactions. Pass the status id; callback fires with success
    // boolean once the server responds.
    void FavouriteStatus(const std::string& statusId, std::function<void(bool success)> callback);
    void UnfavouriteStatus(const std::string& statusId, std::function<void(bool success)> callback);
    void ReblogStatus(const std::string& statusId, std::function<void(bool success)> callback);
    void UnreblogStatus(const std::string& statusId, std::function<void(bool success)> callback);

    bool HasCredentials() const;

    bool Login(const std::string& instance, const std::string& email, const std::string& password);

private:
    std::string m_InstanceURL;
    std::string m_AccessToken;
    AsyncHttpService* m_AsyncHttp;
};

#endif // MASTODON_API_H
