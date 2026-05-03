/*
 * Amidon2 - a Mastodon client for AmigaOS
 * Copyright (C) 2026 Dimitris Panokostas
 */

#include "AsyncHttpService.h"

#include <cstdio>

AsyncHttpService::AsyncHttpService()
    : m_CurrentRequest(nullptr) {
}

AsyncHttpService::~AsyncHttpService() {
    Shutdown();
}

bool AsyncHttpService::Init() {
    return true;
}

void AsyncHttpService::Shutdown() {
    m_CurrentRequest.reset();
    m_Queue.clear();
}

bool AsyncHttpService::IsBusy() const {
    return m_CurrentRequest != nullptr && !m_CurrentRequest->IsFinished();
}

void AsyncHttpService::Get(const std::string& url, const std::string& authHeader,
                             HttpCallback callback) {
    fprintf(stderr, "DEBUG: AsyncHttp::Get url=%s\n", url.c_str());
    QueuedRequest req;
    req.method = "GET";
    req.url = url;
    req.payload = "";
    req.authHeader = authHeader;
    req.contentType = "";
    req.callback = callback;

    if (IsBusy()) {
        m_Queue.push_back(std::move(req));
        return;
    }

    m_CurrentRequest = std::make_unique<HttpRequest>(
        "GET", url, authHeader, "", "", callback);

    fprintf(stderr, "DEBUG: AsyncHttp::Get calling Start()\n");
    if (!m_CurrentRequest->Start()) {
        fprintf(stderr, "DEBUG: AsyncHttp::Get Start() failed\n");
        if (callback) callback("", 0);
        FinishCurrent();
        return;
    }
    fprintf(stderr, "DEBUG: AsyncHttp::Get Start() ok state=%d\n", m_CurrentRequest->GetState());
}

void AsyncHttpService::Post(const std::string& url, const std::string& payload,
                             const std::string& authHeader, const std::string& contentType,
                             HttpCallback callback) {
    if (IsBusy()) {
        QueuedRequest req;
        req.method = "POST";
        req.url = url;
        req.payload = payload;
        req.authHeader = authHeader;
        req.contentType = contentType;
        req.callback = callback;
        m_Queue.push_back(std::move(req));
        return;
    }

    m_CurrentRequest = std::make_unique<HttpRequest>(
        "POST", url, authHeader, contentType, payload, callback);

    if (!m_CurrentRequest->Start()) {
        if (callback) callback("", 0);
        FinishCurrent();
        return;
    }
}

void AsyncHttpService::Progress() {
    if (!m_CurrentRequest) return;

    if (m_CurrentRequest->IsFinished()) {
        FinishCurrent();
        return;
    }

    bool needsMore = m_CurrentRequest->Progress();

    if (m_CurrentRequest->IsFinished()) {
        FinishCurrent();
    } else if (!needsMore) {
        FinishCurrent();
    }
}

int AsyncHttpService::GetSelectFdSets(void* readfds, void* writefds) const {
    if (!m_CurrentRequest || m_CurrentRequest->IsFinished()) {
        return 0;
    }
    return m_CurrentRequest->GetSelectFdSets(readfds, writefds);
}

void AsyncHttpService::StartNext() {
    if (m_Queue.empty()) return;

    QueuedRequest req = std::move(m_Queue.front());
    m_Queue.erase(m_Queue.begin());

    m_CurrentRequest = std::make_unique<HttpRequest>(
        req.method, req.url, req.authHeader, req.contentType, req.payload, req.callback);

    if (!m_CurrentRequest->Start()) {
        if (req.callback) req.callback("", 0);
        FinishCurrent();
    }
}

void AsyncHttpService::FinishCurrent() {
    if (!m_CurrentRequest) return;

    // Move the finished request out before firing its callback so a new
    // request started inside the callback (which sees IsBusy() == false
    // because the finished request reports IsFinished() == true) doesn't
    // get clobbered by m_CurrentRequest.reset() on the way back.
    std::unique_ptr<HttpRequest> finished = std::move(m_CurrentRequest);

    if (finished->GetState() == HttpRequest::STATE_DONE) {
        HttpCallback cb = finished->GetCallback();
        if (cb) {
            cb(finished->GetResponseBody(), finished->GetResponseCode());
        }
    } else if (finished->GetState() == HttpRequest::STATE_ERROR) {
        HttpCallback cb = finished->GetCallback();
        if (cb) {
            cb("", 0);
        }
    }

    // 'finished' destructs at end of scope, closing its socket / SSL.
    // Only drain the queue if the callback didn't already start something.
    if (!m_CurrentRequest) {
        StartNext();
    }
}