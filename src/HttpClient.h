/*
 * Amidon - a Mastodon client for AmigaOS
 * Copyright (C) 2024 Dimitris Panokostas
 */

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>
#include <functional>

using HttpCallback = std::function<void(const std::string&, long)>;

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    void Get(const std::string& url, HttpCallback callback);
    void Post(const std::string& url, const std::string& payload, HttpCallback callback);

    void GetWithHeaders(const std::string& url, const std::string& authHeader, HttpCallback callback);
    void PostWithHeaders(const std::string& url, const std::string& payload,
                         const std::string& contentType, const std::string& authHeader,
                         HttpCallback callback);

private:
    void PerformRequest(const std::string& method, const std::string& url,
                        const std::string& body, const std::string& authHeader,
                        const std::string& contentType, HttpCallback callback);
};

#endif // HTTP_CLIENT_H
