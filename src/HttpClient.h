/*
 * Amidon - a Mastodon client for AmigaOS
 * Copyright (C) 2024 Dimitris Panokostas
 */

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>
#include <functional>
#include <map>

// Callback signature: Body, StatusCode
using HttpCallback = std::function<void(const std::string&, long)>;

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    // Perform a GET request
    void Get(const std::string& url, HttpCallback callback);

    // Perform a POST request
    void Post(const std::string& url, const std::string& payload, HttpCallback callback);

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

#endif // HTTP_CLIENT_H
