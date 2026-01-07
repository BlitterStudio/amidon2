#include "HttpClient.h"
#include <cstdio>

// Stub implementation of HttpClient to remove curl dependency for now
// as requested by the user.

size_t HttpClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    if (userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
    }
    return size * nmemb;
}

HttpClient::HttpClient() {
}

HttpClient::~HttpClient() {
}

void HttpClient::Get(const std::string& url, HttpCallback callback) {
    printf("HttpClient::Get stub called for: %s\n", url.c_str());
    if (callback) {
        callback("", 0); // Return empty for now
    }
}

void HttpClient::Post(const std::string& url, const std::string& payload, HttpCallback callback) {
    printf("HttpClient::Post stub called for: %s\n", url.c_str());
    if (callback) {
        callback("", 0); // Return empty for now
    }
}


