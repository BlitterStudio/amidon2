/*
 * Amidon2 - a Mastodon client for AmigaOS
 * Copyright (C) 2026 Dimitris Panokostas
 *
 * Non-blocking HTTP state machine using WaitSelect.
 * All socket I/O is non-blocking; DNS resolution is blocking (~100-500ms).
 */

#include "HttpRequest.h"
#include "FdSetCompat.h"

#include <sys/filio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

extern "C" {
#include <proto/exec.h>
#include <proto/socket.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>
#include <proto/amisslmaster.h>
#include <proto/amissl.h>
#include <libraries/amisslmaster.h>
#include <libraries/amissl.h>
#include <amissl/amissl.h>
}

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>

namespace {

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define SET_AMISSL_BASES(asBase, amsBase) \
    do { AmiSSLBase = (asBase); AmiSSLMasterBase = (amsBase); } while(0)

const int MAX_REDIRECTS = 5;

bool ParseUrl(const std::string& url, std::string& host, std::string& path,
              unsigned long& port, bool& isHttps) {
    const char* p = url.c_str();
    unsigned long defaultPort = 80;
    isHttps = false;

    if (strncmp(p, "https://", 8) == 0) {
        p += 8;
        defaultPort = 443;
        isHttps = true;
    } else if (strncmp(p, "http://", 7) == 0) {
        p += 7;
    } else {
        return false;
    }

    const char* hs = p;
    while (*p && *p != '/' && *p != ':') p++;

    int n = (int)(p - hs);
    if (n == 0 || n > 255) return false;
    host.assign(hs, n);

    port = defaultPort;
    if (*p == ':') {
        p++;
        port = 0;
        while (*p >= '0' && *p <= '9') {
            port = port * 10 + (*p - '0');
            p++;
        }
        if (port == 0 || port > 65535) return false;
    }

    if (*p == '\0') {
        path = "/";
    } else {
        path = p;
    }
    return true;
}

const char* FindHeaderValue(const char* hdr, const char* name) {
    int nameLen = strlen(name);
    const char* p = hdr;
    while (*p) {
        const char* lineStart = p;
        const char* lineEnd = p;
        while (*lineEnd && *lineEnd != '\r' && *lineEnd != '\n') lineEnd++;

        int lineLen = (int)(lineEnd - lineStart);
        if (lineLen > nameLen + 1 && lineStart[nameLen] == ':' &&
            (lineStart[nameLen + 1] == ' ' || lineStart[nameLen + 1] == '\t')) {
            int match = 1;
            for (int i = 0; i < nameLen; i++) {
                char a = lineStart[i]; char b = name[i];
                if (a >= 'A' && a <= 'Z') a += 32;
                if (b >= 'A' && b <= 'Z') b += 32;
                if (a != b) { match = 0; break; }
            }
            if (match) {
                const char* val = lineStart + nameLen + 1;
                while (*val == ' ' || *val == '\t') val++;
                return val;
            }
        }

        p = lineEnd;
        if (*p == '\r') p++;
        if (*p == '\n') p++;
    }
    return NULL;
}

const char* FindCABundle(void) {
    static const char* const paths[] = {
        "AmiSSL:Certs/curl-ca-bundle.crt",
        "ENVARC:AmiSSL/Certs/curl-ca-bundle.crt",
        "ENV:AmiSSL/Certs/curl-ca-bundle.crt",
        "SYS:Storage/AmiSSL/Certs/curl-ca-bundle.crt",
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        BPTR f = Open((STRPTR)paths[i], MODE_OLDFILE);
        if (f) { Close(f); return paths[i]; }
    }
    return NULL;
}

}

HttpRequest::HttpRequest(const std::string& method, const std::string& url,
                          const std::string& authHeader, const std::string& contentType,
                          const std::string& body, HttpCallback callback)
    : m_Method(method), m_Url(url), m_AuthHeader(authHeader),
      m_ContentType(contentType), m_Body(body), m_Callback(callback),
      m_SocketFd(-1), m_State(STATE_IDLE),
      m_AmiSSLBase(NULL), m_AmiSSLMasterBase(NULL),
      m_SSLCtx(NULL), m_SSL(NULL), m_SSLInitialized(false),
      m_SSLWant(0), m_SendOffset(0), m_ResponseCode(0),
      m_Chunked(false), m_ContentLength(-1), m_ConnectionClose(false),
      m_RedirectCount(0) {
}

HttpRequest::~HttpRequest() {
    CloseSSL();
    CloseConn();
}

bool HttpRequest::Start() {
    m_CurrentUrl = m_Url;

    for (m_RedirectCount = 0; m_RedirectCount < MAX_REDIRECTS; m_RedirectCount++) {
        if (!ParseUrl(m_CurrentUrl, m_Host, m_Path, m_Port, m_IsHttps)) {
            m_State = STATE_ERROR;
            return false;
        }

        struct hostent* he = gethostbyname((STRPTR)m_Host.c_str());
        if (!he) {
            fprintf(stderr, "HttpRequest: DNS failed for '%s'\n", m_Host.c_str());
            m_State = STATE_ERROR;
            return false;
        }

        m_SocketFd = socket(AF_INET, SOCK_STREAM, 0);
        if (m_SocketFd < 0) {
            m_State = STATE_ERROR;
            return false;
        }

        if (!SetNonBlocking(m_SocketFd)) {
            CloseConn();
            m_State = STATE_ERROR;
            return false;
        }

        struct sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons((UWORD)m_Port);
        sin.sin_addr.s_addr = *((ULONG*)he->h_addr_list[0]);

        int ret = connect(m_SocketFd, (struct sockaddr*)&sin, sizeof(sin));
        if (ret < 0) {
            int err = Errno();
            if (err != EINPROGRESS && err != EWOULDBLOCK) {
                CloseConn();
                m_State = STATE_ERROR;
                return false;
            }
        }

        m_State = STATE_CONNECTING;
        return true;
    }

    m_State = STATE_ERROR;
    return false;
}

bool HttpRequest::Progress() {
    switch (m_State) {
        case STATE_CONNECTING:
            return DoConnect();
        case STATE_TLS_HANDSHAKE:
            return DoTlsHandshake();
        case STATE_SENDING:
            return DoSend();
        case STATE_RECV_HEADERS:
            return DoRecvHeaders();
        case STATE_RECV_BODY:
            return DoRecvBody();
        case STATE_REDIRECT:
            return DoRedirect();
        case STATE_DONE:
        case STATE_ERROR:
        case STATE_IDLE:
        case STATE_DNS:
            return false;
    }
    return false;
}

bool HttpRequest::NeedsWrite() const {
    switch (m_State) {
        case STATE_CONNECTING:
        case STATE_SENDING:
        case STATE_TLS_HANDSHAKE:
            return true;
        default:
            return false;
    }
}

int HttpRequest::GetSelectFdSets(void* readfdsVoid, void* writefdsVoid) const {
    if (m_SocketFd < 0 || IsFinished()) return 0;

    amidon_fd_set* readfds = static_cast<amidon_fd_set*>(readfdsVoid);
    amidon_fd_set* writefds = static_cast<amidon_fd_set*>(writefdsVoid);

    AMIDON_FD_ZERO(readfds);
    AMIDON_FD_ZERO(writefds);

    switch (m_State) {
        case STATE_CONNECTING:
            AMIDON_FD_SET(m_SocketFd, writefds);
            break;
        case STATE_SENDING:
            if (m_IsHttps && m_SSL && m_SSLWant == SSL_ERROR_WANT_READ) {
                AMIDON_FD_SET(m_SocketFd, readfds);
            } else {
                AMIDON_FD_SET(m_SocketFd, writefds);
            }
            break;
        case STATE_TLS_HANDSHAKE:
            if (m_SSLWant == SSL_ERROR_WANT_WRITE) {
                AMIDON_FD_SET(m_SocketFd, writefds);
            } else {
                AMIDON_FD_SET(m_SocketFd, readfds);
            }
            break;
        case STATE_RECV_HEADERS:
        case STATE_RECV_BODY:
            if (m_IsHttps && m_SSL && m_SSLWant == SSL_ERROR_WANT_WRITE) {
                AMIDON_FD_SET(m_SocketFd, writefds);
            } else {
                AMIDON_FD_SET(m_SocketFd, readfds);
            }
            break;
        default:
            return 0;
    }

    return m_SocketFd + 1;
}

void HttpRequest::SetError(const char* context) {
    fprintf(stderr, "HttpRequest error: %s (state=%d)\n", context, m_State);
    CloseSSL();
    CloseConn();
    m_State = STATE_ERROR;
}

void HttpRequest::CloseConn() {
    if (m_SocketFd >= 0) {
        LONG fd = m_SocketFd;
        m_SocketFd = -1;
        CloseSocket(fd);
    }
}

void HttpRequest::CloseSSL() {
    if (m_SSL) {
        SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
        SSL_shutdown((SSL*)m_SSL);
        SSL_free((SSL*)m_SSL);
        m_SSL = NULL;
    }
    if (m_SSLCtx) {
        SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
        SSL_CTX_free((SSL_CTX*)m_SSLCtx);
        m_SSLCtx = NULL;
    }
    if (m_SSLInitialized) {
        SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
        CleanupAmiSSLA(NULL);
        m_SSLInitialized = false;
    }
    if (m_AmiSSLBase) {
        SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
        CloseAmiSSL();
        m_AmiSSLBase = NULL;
    }
    if (m_AmiSSLMasterBase) {
        CloseLibrary(m_AmiSSLMasterBase);
        m_AmiSSLMasterBase = NULL;
    }
}

bool HttpRequest::SetNonBlocking(int fd) {
    ULONG on = 1;
    return IoctlSocket(fd, FIONBIO, &on) == 0;
}

int HttpRequest::GetSocketError(int fd) {
    int err = 0;
    LONG len = sizeof(err);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&err, (socklen_t*)&len);
    return err;
}

bool HttpRequest::DoConnect() {
    int err = GetSocketError(m_SocketFd);
    if (err != 0) {
        CloseConn();
        m_State = STATE_ERROR;
        return false;
    }

    if (m_IsHttps) {
        m_AmiSSLMasterBase = OpenLibrary((STRPTR)"amisslmaster.library", AMISSLMASTER_MIN_VERSION);
        if (!m_AmiSSLMasterBase) {
            SetError("amisslmaster.library open failed");
            return false;
        }

        struct Library* savedASBase = NULL;
        struct Library* savedAMSBase = m_AmiSSLMasterBase;

        SET_AMISSL_BASES(savedASBase, savedAMSBase);
        if (!InitAmiSSLMaster(AMISSL_CURRENT_VERSION, TRUE)) {
            CloseLibrary(m_AmiSSLMasterBase);
            m_AmiSSLMasterBase = NULL;
            SetError("InitAmiSSLMaster failed");
            return false;
        }

        SET_AMISSL_BASES(savedASBase, savedAMSBase);
        m_AmiSSLBase = OpenAmiSSL();
        if (!m_AmiSSLBase) {
            CloseLibrary(m_AmiSSLMasterBase);
            m_AmiSSLMasterBase = NULL;
            SetError("OpenAmiSSL failed");
            return false;
        }

        int sniErrno = 0;
        struct TagItem initTags[] = {
            { AmiSSL_ErrNoPtr, (ULONG)&sniErrno },
            { AmiSSL_SocketBase, (ULONG)SocketBase },
            { TAG_DONE, 0 }
        };
        SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
        if (InitAmiSSLA(initTags) != 0) {
            SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
            CloseAmiSSL();
            m_AmiSSLBase = NULL;
            CloseLibrary(m_AmiSSLMasterBase);
            m_AmiSSLMasterBase = NULL;
            SetError("InitAmiSSLA failed");
            return false;
        }
        m_SSLInitialized = true;

        SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
        OPENSSL_init_ssl(OPENSSL_INIT_SSL_DEFAULT | OPENSSL_INIT_ADD_ALL_CIPHERS | OPENSSL_INIT_ADD_ALL_DIGESTS, NULL);

        SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
        {
            unsigned char seed[32];
            ULONG t = (ULONG)FindTask(NULL);
            for (ULONG i = 0; i < sizeof(seed); i++)
                seed[i] = (unsigned char)(t >> ((i & 3) * 8)) ^ (unsigned char)i;
            RAND_seed(seed, sizeof(seed));
        }

        SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
        m_SSLCtx = SSL_CTX_new(TLS_client_method());
        if (!m_SSLCtx) {
            SetError("SSL_CTX_new failed");
            return false;
        }

        const char* caBundle = FindCABundle();
        SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
        if (caBundle) {
            if (SSL_CTX_load_verify_locations((SSL_CTX*)m_SSLCtx, caBundle, NULL) != 1) {
                SSL_CTX_set_verify((SSL_CTX*)m_SSLCtx, SSL_VERIFY_NONE, NULL);
            } else {
                SSL_CTX_set_verify((SSL_CTX*)m_SSLCtx, SSL_VERIFY_PEER, NULL);
            }
        } else {
            SSL_CTX_set_verify((SSL_CTX*)m_SSLCtx, SSL_VERIFY_NONE, NULL);
        }

        SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
        m_SSL = SSL_new((SSL_CTX*)m_SSLCtx);
        if (!m_SSL) {
            SetError("SSL_new failed");
            return false;
        }

        SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
        SSL_set_fd((SSL*)m_SSL, m_SocketFd);
        SSL_set_tlsext_host_name((SSL*)m_SSL, m_Host.c_str());

        if (caBundle) {
            SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
            SSL_set_hostflags((SSL*)m_SSL, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
            SSL_set1_host((SSL*)m_SSL, m_Host.c_str());
        }

        m_State = STATE_TLS_HANDSHAKE;
        m_SSLWant = SSL_ERROR_WANT_WRITE;
        return DoTlsHandshake();
    }

    m_SendBuffer = BuildRequest();
    m_SendOffset = 0;
    m_ResponseHeaders.clear();
    m_ResponseBody.clear();
    m_State = STATE_SENDING;
    m_SSLWant = 0;
    return DoSend();
}

bool HttpRequest::DoTlsHandshake() {
    SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
    int ret = SSL_connect((SSL*)m_SSL);

    if (ret == 1) {
        m_SSLWant = 0;
        m_SendBuffer = BuildRequest();
        m_SendOffset = 0;
        m_ResponseHeaders.clear();
        m_ResponseBody.clear();
        m_State = STATE_SENDING;
        return true;
    }

    int sslErr = SSL_get_error((SSL*)m_SSL, ret);
    if (sslErr == SSL_ERROR_WANT_READ) {
        m_SSLWant = SSL_ERROR_WANT_READ;
        return true;
    }
    if (sslErr == SSL_ERROR_WANT_WRITE) {
        m_SSLWant = SSL_ERROR_WANT_WRITE;
        return true;
    }

    SetError("SSL_connect failed");
    return false;
}

bool HttpRequest::DoSend() {
    const char* data = m_SendBuffer.data() + m_SendOffset;
    size_t remaining = m_SendBuffer.size() - m_SendOffset;

    while (remaining > 0) {
        int sent;
        if (m_IsHttps && m_SSL) {
            SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
            sent = SSL_write((SSL*)m_SSL, data, (int)remaining);
            if (sent <= 0) {
                int sslErr = SSL_get_error((SSL*)m_SSL, sent);
                if (sslErr == SSL_ERROR_WANT_READ || sslErr == SSL_ERROR_WANT_WRITE) {
                    m_SSLWant = sslErr;
                    return true;
                }
                SetError("SSL_write failed during send");
                return false;
            }
        } else {
            sent = send(m_SocketFd, (APTR)data, (int)remaining, 0);
            if (sent <= 0) {
                int err = Errno();
                if (err == EWOULDBLOCK || err == EAGAIN) {
                    return true;
                }
                SetError("send() failed");
                return false;
            }
        }
        m_SendOffset += sent;
        data = m_SendBuffer.data() + m_SendOffset;
        remaining = m_SendBuffer.size() - m_SendOffset;
    }

    if (!m_Body.empty()) {
        m_SendBuffer = m_Body;
        m_SendOffset = 0;
        size_t bodyRemaining = m_SendBuffer.size();
        const char* bodyData = m_SendBuffer.data();

        while (bodyRemaining > 0) {
            int sent;
            if (m_IsHttps && m_SSL) {
                SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
                sent = SSL_write((SSL*)m_SSL, bodyData, (int)bodyRemaining);
                if (sent <= 0) {
                    int sslErr = SSL_get_error((SSL*)m_SSL, sent);
                    if (sslErr == SSL_ERROR_WANT_READ || sslErr == SSL_ERROR_WANT_WRITE) {
                        m_SendOffset = bodyData - m_SendBuffer.data();
                        m_SSLWant = sslErr;
                        return true;
                    }
                    SetError("SSL_write failed during body send");
                    return false;
                }
            } else {
                sent = send(m_SocketFd, (APTR)bodyData, (int)bodyRemaining, 0);
                if (sent <= 0) {
                    int err = Errno();
                    if (err == EWOULDBLOCK || err == EAGAIN) {
                        return true;
                    }
                    SetError("send() failed during body");
                    return false;
                }
            }
            bodyData += sent;
            bodyRemaining -= sent;
        }
    }

    m_RecvBuffer.resize(16384);
    m_RecvOffset = 0;
    m_ResponseHeaders.clear();
    m_ResponseBody.clear();
    m_State = STATE_RECV_HEADERS;
    m_SSLWant = SSL_ERROR_WANT_READ;
    return true;
}

bool HttpRequest::DoRecvHeaders() {
    while (true) {
        int got;
        if (m_IsHttps && m_SSL) {
            SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
            got = SSL_read((SSL*)m_SSL, m_RecvBuffer.data(), (int)m_RecvBuffer.size());
            if (got <= 0) {
                int sslErr = SSL_get_error((SSL*)m_SSL, got);
                if (sslErr == SSL_ERROR_WANT_READ || sslErr == SSL_ERROR_WANT_WRITE) {
                    m_SSLWant = sslErr;
                    return true;
                }
                if (sslErr == SSL_ERROR_ZERO_RETURN || sslErr == SSL_ERROR_SYSCALL) {
                    // Connection closed by server
                    break;
                }
                SetError("SSL_read failed during headers");
                return false;
            }
        } else {
            got = recv(m_SocketFd, m_RecvBuffer.data(), (int)m_RecvBuffer.size(), 0);
            if (got <= 0) {
                if (got < 0) {
                    int err = Errno();
                    if (err == EWOULDBLOCK || err == EAGAIN) {
                        return true;
                    }
                    SetError("recv() failed during headers");
                    return false;
                }
                break;
            }
        }

        m_ResponseHeaders.append(m_RecvBuffer.data(), got);

        const char* hdrEnd = strstr(m_ResponseHeaders.c_str(), "\r\n\r\n");
        if (hdrEnd) {
            int headerBlockSize = (int)(hdrEnd - m_ResponseHeaders.c_str()) + 4;
            int bodyAlready = m_ResponseHeaders.size() - headerBlockSize;
            if (bodyAlready > 0) {
                m_ResponseBody.append(hdrEnd + 4, bodyAlready);
            }

            const char* hdrStr = m_ResponseHeaders.c_str();
            const char* sp = strchr(hdrStr, ' ');
            m_ResponseCode = 0;
            if (sp) {
                while (*sp == ' ') sp++;
                while (*sp >= '0' && *sp <= '9') {
                    m_ResponseCode = m_ResponseCode * 10 + (*sp - '0');
                    sp++;
                }
            }

            if (m_ResponseCode == 301 || m_ResponseCode == 302 ||
                m_ResponseCode == 303 || m_ResponseCode == 307 || m_ResponseCode == 308) {
                m_State = STATE_REDIRECT;
                return DoRedirect();
            }

            m_Chunked = (strstr(hdrStr, "Transfer-Encoding: chunked") != NULL ||
                         strstr(hdrStr, "transfer-encoding: chunked") != NULL);

            const char* clStr = FindHeaderValue(hdrStr, "Content-Length");
            m_ContentLength = -1;
            if (clStr) {
                m_ContentLength = atol(clStr);
            }

            m_ConnectionClose = (strstr(hdrStr, "Connection: close") != NULL ||
                                 strstr(hdrStr, "connection: close") != NULL);

            m_State = STATE_RECV_BODY;
            m_SSLWant = SSL_ERROR_WANT_READ;
            return true;
        }

        if (m_ResponseHeaders.size() > 32768) {
            SetError("headers too large");
            return false;
        }
    }

    SetError("connection closed before headers complete");
    return false;
}

bool HttpRequest::DoRecvBody() {
    while (true) {
        int got;
        if (m_IsHttps && m_SSL) {
            SET_AMISSL_BASES(m_AmiSSLBase, m_AmiSSLMasterBase);
            got = SSL_read((SSL*)m_SSL, m_RecvBuffer.data(), (int)m_RecvBuffer.size());
            if (got <= 0) {
                int sslErr = SSL_get_error((SSL*)m_SSL, got);
                if (sslErr == SSL_ERROR_WANT_READ || sslErr == SSL_ERROR_WANT_WRITE) {
                    m_SSLWant = sslErr;
                    return true;
                }
                if (sslErr == SSL_ERROR_ZERO_RETURN || sslErr == SSL_ERROR_SYSCALL) {
                    // Server closed connection — for non-chunked with no Content-Length, this means done
                    break;
                }
                SetError("SSL_read failed during body");
                return false;
            }
        } else {
            got = recv(m_SocketFd, m_RecvBuffer.data(), (int)m_RecvBuffer.size(), 0);
            if (got <= 0) {
                if (got < 0) {
                    int err = Errno();
                    if (err == EWOULDBLOCK || err == EAGAIN) {
                        return true;
                    }
                    SetError("recv() failed during body");
                    return false;
                }
                break;
            }
        }

        m_ResponseBody.append(m_RecvBuffer.data(), got);

        if (m_Chunked) {
            if (m_ResponseBody.find("0\r\n\r\n") != std::string::npos) {
                break;
            }
        } else if (m_ContentLength >= 0) {
            if ((long)m_ResponseBody.size() >= m_ContentLength) {
                break;
            }
        }

        if (m_ResponseBody.size() > 5 * 1024 * 1024) {
            SetError("response body too large");
            return false;
        }
    }

    CloseSSL();
    CloseConn();

    m_State = STATE_DONE;
    return false;
}

bool HttpRequest::DoRedirect() {
    const char* hdrStr = m_ResponseHeaders.c_str();
    const char* loc = FindHeaderValue(hdrStr, "Location");
    if (!loc) {
        SetError("redirect without Location header");
        return false;
    }

    std::string newUrl;
    const char* locEnd = loc;
    while (*locEnd && *locEnd != '\r' && *locEnd != '\n') locEnd++;
    newUrl.assign(loc, (size_t)(locEnd - loc));

    CloseSSL();
    CloseConn();

    m_CurrentUrl = newUrl;
    m_RedirectCount++;

    return Start();
}

std::string HttpRequest::BuildRequest() {
    std::string req;
    req.reserve(4096);

    req += m_Method;
    req += ' ';
    req += m_Path;
    req += " HTTP/1.1\r\n";
    req += "Host: ";
    req += m_Host;
    req += "\r\n";
    req += "User-Agent: Amidon/0.2\r\n";
    req += "Accept: */*\r\n";
    req += "Connection: close\r\n";

    if (!m_AuthHeader.empty()) {
        req += m_AuthHeader;
        req += "\r\n";
    }

    if (m_Method == "POST" && !m_Body.empty()) {
        const char* ct = m_ContentType.empty() ? "application/json" : m_ContentType.c_str();
        req += "Content-Type: ";
        req += ct;
        req += "\r\n";
        char lenBuf[32];
        snprintf(lenBuf, sizeof(lenBuf), "%lu", (unsigned long)m_Body.size());
        req += "Content-Length: ";
        req += lenBuf;
        req += "\r\n";
    }

    req += "\r\n";
    return req;
}