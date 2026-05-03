/*
 * Amidon2 - a Mastodon client for AmigaOS
 * Copyright (C) 2026 Dimitris Panokostas
 */

#include "MastodonAPI.h"
#include "logic/TimelineParser.h"
#include "logic/AccountParser.h"
#include "logic/NotificationParser.h"
#include "logic/UrlBuilder.h"
#include "logic/CredsParser.h"

#include <cstdio>

MastodonAPI::MastodonAPI() : m_AsyncHttp(nullptr) {
}

void MastodonAPI::SetAsyncHttp(AsyncHttpService* asyncHttp) {
    m_AsyncHttp = asyncHttp;
}

void MastodonAPI::RegisterApp(const std::string& instance,
    std::function<void(bool success, const AppRegistration& reg)> callback) {
    std::string url = UrlBuilder::RegisterApp(instance);

    json_object* payload = json_object_new_object();
    json_object_object_add(payload, "client_name", json_object_new_string("Amidon"));
    json_object_object_add(payload, "redirect_uris", json_object_new_string("urn:ietf:wg:oauth:2.0:oob"));
    json_object_object_add(payload, "scopes", json_object_new_string("read write follow push"));
    std::string payloadStr = json_object_to_json_string(payload);

    auto handleResponse = [callback, instance](const std::string& response, long code) {
        AppRegistration reg;
        if (CredsParser::ParseRegistration(response, instance, reg)) {
            if (callback) callback(true, reg);
        } else {
            fprintf(stderr, "RegisterApp: JSON parse error\n");
            if (callback) callback(false, {});
        }
    };

    if (!m_AsyncHttp) {
        fprintf(stderr, "RegisterApp: AsyncHttp service unavailable\n");
        if (callback) callback(false, {});
        json_object_put(payload);
        return;
    }
    m_AsyncHttp->Post(url, payloadStr, "", "application/json", handleResponse);
    json_object_put(payload);
}

void MastodonAPI::GetToken(const AppRegistration& reg, const std::string& authCode,
    std::function<void(bool success, const std::string& token)> callback) {
    std::string url = UrlBuilder::OAuthToken(reg.instance_url);

    json_object* payload = json_object_new_object();
    json_object_object_add(payload, "client_id", json_object_new_string(reg.client_id.c_str()));
    json_object_object_add(payload, "client_secret", json_object_new_string(reg.client_secret.c_str()));
    json_object_object_add(payload, "redirect_uri", json_object_new_string("urn:ietf:wg:oauth:2.0:oob"));
    json_object_object_add(payload, "grant_type", json_object_new_string("authorization_code"));
    json_object_object_add(payload, "code", json_object_new_string(authCode.c_str()));
    std::string payloadStr = json_object_to_json_string(payload);

    auto handleResponse = [callback](const std::string& response, long code) {
        if (code != 200) {
            fprintf(stderr, "GetToken failed with code: %ld\n", code);
            if (callback) callback(false, "");
            return;
        }

        std::string accessToken;
        if (CredsParser::ParseToken(response, accessToken)) {
            if (callback) callback(true, accessToken);
        } else {
            fprintf(stderr, "GetToken: JSON parse error\n");
            if (callback) callback(false, "");
        }
    };

    if (!m_AsyncHttp) {
        fprintf(stderr, "GetToken: AsyncHttp service unavailable\n");
        if (callback) callback(false, "");
        json_object_put(payload);
        return;
    }
    m_AsyncHttp->Post(url, payloadStr, "", "application/json", handleResponse);
    json_object_put(payload);
}

void MastodonAPI::SetCredentials(const std::string& instance, const std::string& token) {
    m_InstanceURL = instance;
    m_AccessToken = token;
}

void MastodonAPI::GetHomeTimeline(std::function<void(std::vector<Status>)> callback) {
    if (m_InstanceURL.empty() || m_AccessToken.empty()) {
        if (callback) callback({});
        return;
    }

    std::string url = UrlBuilder::TimelineHome(m_InstanceURL);
    std::string authHeader = UrlBuilder::AuthHeader(m_AccessToken);

    auto parseTimeline = [callback](const std::string& response, long code) {
        if (code != 200) {
            fprintf(stderr, "GetHomeTimeline failed with code: %ld\n", code);
            if (callback) callback({});
            return;
        }

        if (callback) callback(TimelineParser::ParseTimeline(response));
    };

    if (!m_AsyncHttp) {
        fprintf(stderr, "GetHomeTimeline: AsyncHttp service unavailable\n");
        if (callback) callback({});
        return;
    }
    m_AsyncHttp->Get(url, authHeader, parseTimeline);
}

void MastodonAPI::GetPublicTimeline(std::function<void(std::vector<Status>)> callback) {
    if (m_InstanceURL.empty()) {
        if (callback) callback({});
        return;
    }

    std::string url = UrlBuilder::TimelinePublic(m_InstanceURL);

    auto parseTimeline = [callback](const std::string& response, long code) {
        if (code != 200) {
            fprintf(stderr, "GetPublicTimeline failed code=%ld body=%.200s\n",
                    code, response.c_str());
            if (callback) callback({});
            return;
        }

        if (callback) callback(TimelineParser::ParseTimeline(response));
    };

    if (!m_AsyncHttp) {
        fprintf(stderr, "GetPublicTimeline: AsyncHttp service unavailable\n");
        if (callback) callback({});
        return;
    }
    m_AsyncHttp->Get(url, "", parseTimeline);
}

namespace {

// Shared parser/dispatcher for endpoints that return a JSON array of Status.
void RunStatusListGet(AsyncHttpService* asyncHttp, const std::string& url,
                       const std::string& authHeader, const char* tag,
                       std::function<void(std::vector<Status>)> callback) {
    auto handler = [callback, tag](const std::string& response, long code) {
        if (code != 200) {
            fprintf(stderr, "%s failed code=%ld body=%.200s\n",
                    tag, code, response.c_str());
            if (callback) callback({});
            return;
        }
        if (callback) callback(TimelineParser::ParseTimeline(response));
    };

    if (!asyncHttp) {
        fprintf(stderr, "%s: AsyncHttp service unavailable\n", tag);
        if (callback) callback({});
        return;
    }
    asyncHttp->Get(url, authHeader, handler);
}

}  // namespace

void MastodonAPI::GetFavourites(std::function<void(std::vector<Status>)> callback) {
    if (m_InstanceURL.empty() || m_AccessToken.empty()) {
        if (callback) callback({});
        return;
    }
    RunStatusListGet(m_AsyncHttp,
                      UrlBuilder::Favourites(m_InstanceURL),
                      UrlBuilder::AuthHeader(m_AccessToken),
                      "GetFavourites", callback);
}

void MastodonAPI::GetBookmarks(std::function<void(std::vector<Status>)> callback) {
    if (m_InstanceURL.empty() || m_AccessToken.empty()) {
        if (callback) callback({});
        return;
    }
    RunStatusListGet(m_AsyncHttp,
                      UrlBuilder::Bookmarks(m_InstanceURL),
                      UrlBuilder::AuthHeader(m_AccessToken),
                      "GetBookmarks", callback);
}

namespace {

void RunStatusActionPost(AsyncHttpService* asyncHttp, const std::string& url,
                         const std::string& authHeader, const char* tag,
                         std::function<void(bool success)> callback) {
    auto handler = [callback, tag](const std::string& response, long code) {
        if (code != 200) {
            fprintf(stderr, "%s failed code=%ld body=%.200s\n",
                    tag, code, response.c_str());
            if (callback) callback(false);
            return;
        }
        if (callback) callback(true);
    };

    if (!asyncHttp) {
        fprintf(stderr, "%s: AsyncHttp service unavailable\n", tag);
        if (callback) callback(false);
        return;
    }
    // The Mastodon /favourite, /reblog etc. endpoints take no body but do
    // require the auth header. Send an empty JSON object so curl-style
    // intermediaries don't reject a missing Content-Type/Length combo.
    asyncHttp->Post(url, "{}", authHeader, "application/json", handler);
}

}  // namespace

void MastodonAPI::FavouriteStatus(const std::string& statusId,
                                    std::function<void(bool success)> callback) {
    if (statusId.empty() || m_InstanceURL.empty() || m_AccessToken.empty()) {
        if (callback) callback(false);
        return;
    }
    RunStatusActionPost(m_AsyncHttp,
                         UrlBuilder::FavouriteStatus(m_InstanceURL, statusId),
                         UrlBuilder::AuthHeader(m_AccessToken),
                         "FavouriteStatus", callback);
}

void MastodonAPI::UnfavouriteStatus(const std::string& statusId,
                                      std::function<void(bool success)> callback) {
    if (statusId.empty() || m_InstanceURL.empty() || m_AccessToken.empty()) {
        if (callback) callback(false);
        return;
    }
    RunStatusActionPost(m_AsyncHttp,
                         UrlBuilder::UnfavouriteStatus(m_InstanceURL, statusId),
                         UrlBuilder::AuthHeader(m_AccessToken),
                         "UnfavouriteStatus", callback);
}

void MastodonAPI::ReblogStatus(const std::string& statusId,
                                 std::function<void(bool success)> callback) {
    if (statusId.empty() || m_InstanceURL.empty() || m_AccessToken.empty()) {
        if (callback) callback(false);
        return;
    }
    RunStatusActionPost(m_AsyncHttp,
                         UrlBuilder::ReblogStatus(m_InstanceURL, statusId),
                         UrlBuilder::AuthHeader(m_AccessToken),
                         "ReblogStatus", callback);
}

void MastodonAPI::UnreblogStatus(const std::string& statusId,
                                   std::function<void(bool success)> callback) {
    if (statusId.empty() || m_InstanceURL.empty() || m_AccessToken.empty()) {
        if (callback) callback(false);
        return;
    }
    RunStatusActionPost(m_AsyncHttp,
                         UrlBuilder::UnreblogStatus(m_InstanceURL, statusId),
                         UrlBuilder::AuthHeader(m_AccessToken),
                         "UnreblogStatus", callback);
}

void MastodonAPI::GetNotifications(std::function<void(std::vector<Notification>)> callback) {
    if (m_InstanceURL.empty() || m_AccessToken.empty()) {
        if (callback) callback({});
        return;
    }

    std::string url = UrlBuilder::Notifications(m_InstanceURL);
    std::string authHeader = UrlBuilder::AuthHeader(m_AccessToken);

    auto parseNotifs = [callback](const std::string& response, long code) {
        if (code != 200) {
            fprintf(stderr, "GetNotifications failed code=%ld body=%.200s\n",
                    code, response.c_str());
            if (callback) callback({});
            return;
        }
        if (callback) callback(NotificationParser::ParseNotifications(response));
    };

    if (!m_AsyncHttp) {
        fprintf(stderr, "GetNotifications: AsyncHttp service unavailable\n");
        if (callback) callback({});
        return;
    }
    m_AsyncHttp->Get(url, authHeader, parseNotifs);
}

void MastodonAPI::PostStatus(const std::string& content, const std::string& visibility,
    const std::string& spoilerText,
    std::function<void(bool success, const std::string& statusId)> callback) {
    if (m_InstanceURL.empty() || m_AccessToken.empty()) {
        if (callback) callback(false, "");
        return;
    }

    std::string url = "https://" + m_InstanceURL + "/api/v1/statuses";

    json_object* payload = json_object_new_object();
    json_object_object_add(payload, "status", json_object_new_string(content.c_str()));
    json_object_object_add(payload, "visibility", json_object_new_string(visibility.c_str()));
    if (!spoilerText.empty())
        json_object_object_add(payload, "spoiler_text", json_object_new_string(spoilerText.c_str()));
    const char* payloadStr = json_object_to_json_string(payload);

    std::string authHeader = UrlBuilder::AuthHeader(m_AccessToken);

    auto handlePostResponse = [callback](const std::string& response, long code) {
        if (code != 200 && code != 201) {
            fprintf(stderr, "PostStatus failed with code: %ld\n", code);
            if (callback) callback(false, "");
            return;
        }

        json_object* root = json_tokener_parse(response.c_str());
        if (!root) {
            fprintf(stderr, "PostStatus: JSON parse error\n");
            if (callback) callback(false, "");
            return;
        }

        json_object* val = NULL;
        if (json_object_object_get_ex(root, "id", &val) && json_object_is_type(val, json_type_string)) {
            std::string id = json_object_get_string(val);
            json_object_put(root);
            if (callback) callback(true, id);
        } else {
            json_object_put(root);
            if (callback) callback(false, "");
        }
    };

    if (!m_AsyncHttp) {
        fprintf(stderr, "PostStatus: AsyncHttp service unavailable\n");
        if (callback) callback(false, "");
        json_object_put(payload);
        return;
    }
    m_AsyncHttp->Post(url, payloadStr, authHeader, "application/json", handlePostResponse);
    json_object_put(payload);
}

void MastodonAPI::GetAccountInfo(std::function<void(bool success, const Account& account)> callback) {
    if (m_InstanceURL.empty() || m_AccessToken.empty()) {
        if (callback) callback(false, {});
        return;
    }

    std::string url = UrlBuilder::AccountVerify(m_InstanceURL);
    std::string authHeader = UrlBuilder::AuthHeader(m_AccessToken);

    auto parseAccount = [callback](const std::string& response, long code) {
        if (code != 200) {
            fprintf(stderr, "GetAccountInfo failed with code: %ld\n", code);
            if (callback) callback(false, {});
            return;
        }

        Account acct = AccountParser::ParseAccount(response);
        if (acct.id.empty()) {
            if (callback) callback(false, {});
            return;
        }
        if (callback) callback(true, acct);
    };

    if (!m_AsyncHttp) {
        fprintf(stderr, "GetAccountInfo: AsyncHttp service unavailable\n");
        if (callback) callback(false, {});
        return;
    }
    m_AsyncHttp->Get(url, authHeader, parseAccount);
}

bool MastodonAPI::HasCredentials() const {
    return !m_InstanceURL.empty() && !m_AccessToken.empty();
}

bool MastodonAPI::Login(const std::string& instance, const std::string& email, const std::string& password) {
    return false;
}
