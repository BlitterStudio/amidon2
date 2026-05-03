/*
 * Amidon2 - a Mastodon client for AmigaOS
 * Copyright (C) 2026 Dimitris Panokostas
 */

#ifndef ASYNC_HTTP_SERVICE_H
#define ASYNC_HTTP_SERVICE_H

extern "C" {
#include <exec/types.h>
#include <exec/ports.h>
}

#include "HttpClient.h"

#include <string>
#include <vector>

struct AsyncHttpWork {
    struct Message msg;
    char* url;
    char* method;
    char* payload;
    char* authHeader;
    char* contentType;
    char* responseBody;
    long responseCode;
    int success;
    struct MsgPort* replyPort;
};

class AsyncHttpService {
public:
    AsyncHttpService();
    ~AsyncHttpService();

    bool Init();
    void Shutdown();
    ULONG SignalMask() const;
    bool IsBusy() const;

    void Get(const std::string& url, const std::string& authHeader, HttpCallback callback);
    void Post(const std::string& url, const std::string& payload,
              const std::string& authHeader, const std::string& contentType,
              HttpCallback callback);

    void HandleCompletion();

private:
    bool StartRequest(const char* method, const char* url, const char* payload,
                      const char* authHeader, const char* contentType,
                      HttpCallback callback);
    void ProcessQueue();

    struct MsgPort* m_ReplyPort;
    ULONG m_ReplySig;
    bool m_Busy;
    HttpCallback m_Callback;
    AsyncHttpWork* m_CurrentWork;

    struct QueuedRequest {
        std::string method;
        std::string url;
        std::string payload;
        std::string authHeader;
        std::string contentType;
        HttpCallback callback;
    };
    std::vector<QueuedRequest> m_Queue;
};

#endif // ASYNC_HTTP_SERVICE_H
