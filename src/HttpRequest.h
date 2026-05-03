/*
 * Amidon2 - a Mastodon client for AmigaOS
 * Copyright (C) 2026 Dimitris Panokostas
 */

#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <functional>
#include <vector>

extern "C" {
#include <exec/types.h>
}

/*
 * HttpRequest: non-blocking HTTP state machine using WaitSelect.
 *
 * Lifecycle:
 *   1. Create with method, url, headers, body, callback
 *   2. Call Start() to initiate DNS + connect
 *   3. Call Progress() repeatedly from the MUI event loop whenever
 *      the socket becomes readable/writable (signalled via WaitSelect)
 *   4. The state machine advances through: DNS -> CONNECT -> TLS -> SEND -> RECV -> DONE
 *   5. On completion or error, the callback fires and the request is finished
 *
 * All socket I/O is non-blocking. Progress() never blocks.
 * DNS resolution (gethostbyname) is the only blocking step (~100-500ms).
 */

using HttpCallback = std::function<void(const std::string& responseBody, long statusCode)>;

class HttpRequest {
public:
    enum State {
        STATE_IDLE,
        STATE_DNS,
        STATE_CONNECTING,
        STATE_TLS_HANDSHAKE,
        STATE_SENDING,
        STATE_RECV_HEADERS,
        STATE_RECV_BODY,
        STATE_REDIRECT,
        STATE_DONE,
        STATE_ERROR
    };

    HttpRequest(const std::string& method, const std::string& url,
                const std::string& authHeader, const std::string& contentType,
                const std::string& body, HttpCallback callback);
    ~HttpRequest();

    State GetState() const { return m_State; }
    bool IsFinished() const { return m_State == STATE_DONE || m_State == STATE_ERROR; }
    long GetResponseCode() const { return m_ResponseCode; }
    const std::string& GetResponseBody() const { return m_ResponseBody; }
    HttpCallback GetCallback() const { return m_Callback; }

    /*
     * Start the request. Performs blocking DNS (gethostbyname),
     * then sets socket to non-blocking and initiates connect().
     * Returns false on immediate failure (DNS error, socket creation error).
     */
    bool Start();

    /*
     * Advance the state machine. Call this when WaitSelect reports
     * the socket is readable or writable.
     * Returns true if the request needs more Progress() calls,
     * false if it's finished (check IsFinished()).
     */
    bool Progress();

    /*
     * Return the socket file descriptor for use with WaitSelect.
     * Only valid after Start() returns true and before IsFinished().
     * Returns -1 if no active socket.
     */
    int GetSocketFd() const { return m_SocketFd; }

    /*
     * Returns true if Progress() needs the socket to be writable.
     * Returns false if it needs the socket to be readable.
     */
    bool NeedsWrite() const;

    /*
     * Get the socket fd_set bits for WaitSelect.
     * Sets the appropriate fd in readfds or writefds based on current state.
     * Returns the nfds value (max fd + 1) for WaitSelect.
     * Returns 0 if no socket is active.
     */
    int GetSelectFdSets(void* readfds, void* writefds) const;

private:
    void SetError(const char* context);
    void CloseConn();
    void CloseSSL();

    bool DoConnect();
    bool DoTlsHandshake();
    bool DoSend();
    bool DoRecvHeaders();
    bool DoRecvBody();
    bool DoRedirect();

    // Non-blocking socket helpers
    bool SetNonBlocking(int fd);
    int GetSocketError(int fd);
    std::string BuildRequest();

    // Parsed URL components
    std::string m_Host;
    std::string m_Path;
    unsigned long m_Port;
    bool m_IsHttps;

    // Request data
    std::string m_Method;
    std::string m_Url;
    std::string m_AuthHeader;
    std::string m_ContentType;
    std::string m_Body;
    std::string m_CurrentUrl;
    HttpCallback m_Callback;

    // Socket and SSL state
    int m_SocketFd;
    State m_State;

    // AmiSSL state (local pointers, not globals)
    struct Library* m_AmiSSLBase;
    struct Library* m_AmiSSLMasterBase;
    void* m_SSLCtx;
    void* m_SSL;
    bool m_SSLInitialized;
    int m_SSLWant;  // 0=none, SSL_ERROR_WANT_READ, SSL_ERROR_WANT_WRITE
    int m_ErrNo;    // ErrNo storage for AmiSSL — must outlive DoConnect() stack frame

    // Send/receive buffers
    std::string m_SendBuffer;
    size_t m_SendOffset;
    std::vector<char> m_RecvBuffer;
    size_t m_RecvOffset;

    // Response
    std::string m_ResponseHeaders;
    std::string m_ResponseBody;
    long m_ResponseCode;
    bool m_Chunked;
    long m_ContentLength;
    bool m_ConnectionClose;

    // Redirect counter
    int m_RedirectCount;
    static const int MAX_REDIRECTS = 5;
};

#endif // HTTP_REQUEST_H