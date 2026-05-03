/*
 * Amidon2 - a Mastodon client for AmigaOS
 * Copyright (C) 2026 Dimitris Panokostas
 */

#ifndef ASYNC_HTTP_SERVICE_H
#define ASYNC_HTTP_SERVICE_H

extern "C" {
#include <exec/types.h>
}

#include "HttpRequest.h"

#include <string>
#include <vector>
#include <memory>

class AsyncHttpService {
public:
    AsyncHttpService();
    ~AsyncHttpService();

    bool Init();
    void Shutdown();

    bool IsBusy() const;

    void Get(const std::string& url, const std::string& authHeader, HttpCallback callback);
    void Post(const std::string& url, const std::string& payload,
              const std::string& authHeader, const std::string& contentType,
              HttpCallback callback);

    /*
     * Advance the active request's state machine.
     * Call from the MUI event loop when WaitSelect reports
     * the socket is ready.
     */
    void Progress();

    /*
     * Build a WaitSelect-compatible signal mask.
     * Returns the socket fd if a request is active and needs
     * WaitSelect attention, or -1 if idle.
     * Populates readfds/writefds based on the request's current state.
     */
    int GetSelectFdSets(fd_set* readfds, fd_set* writefds) const;

private:
    void StartNext();
    void FinishCurrent();

    std::unique_ptr<HttpRequest> m_CurrentRequest;

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