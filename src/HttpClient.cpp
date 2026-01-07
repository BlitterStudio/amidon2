#include "HttpClient.h"
#include <cstdio>
#include <curl/curl.h>

size_t HttpClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    if (userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
    }
    return size * nmemb;
}

HttpClient::HttpClient() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

HttpClient::~HttpClient() {
    curl_global_cleanup();
}

void HttpClient::Get(const std::string& url, HttpCallback callback) {
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string readBuffer;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // Enable verbose debug output
        
        // SSL verification might be tricky on older systems, but let's try defaults first
        // If AmiSSL is set up correctly, it should work.
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Disabled for testing/emulation
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        CURLcode res = curl_easy_perform(curl);
        long response_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (res != CURLE_OK) {
             fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
             if (callback) callback("", 0);
        } else {
             if (callback) callback(readBuffer, (int)response_code);
        }
        curl_easy_cleanup(curl);
    } else {
        if (callback) callback("", 0);
    }
}

void HttpClient::Post(const std::string& url, const std::string& payload, HttpCallback callback) {
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string readBuffer;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        long response_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
             if (callback) callback("", 0);
        } else {
             if (callback) callback(readBuffer, (int)response_code);
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        if (callback) callback("", 0);
    }
}

