/*
 * Amidon2 - a Mastodon client for AmigaOS
 * Copyright (C) 2026 Dimitris Panokostas
 *
 * Non-blocking HTTP state machine using WaitSelect.
 * All socket I/O is non-blocking; DNS resolution is blocking (~100-500ms).
 */

#include "HttpRequest.h"

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
#include <sys/filio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
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
      m_SendOffset(0), m_ResponseCode(0),
      m_Chunked(false), m_ContentLength(-1), m_ConnectionClose(false),
      m_RedirectCount(0) {
}

HttpRequest::~HttpRequest() {
    CloseSSL();
    CloseSocket();
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
            CloseSocket();
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
                CloseSocket();
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

int HttpRequest::GetSelectFdSets(fd_set* readfds, fd_set* writefds) const {
    if (m_SocketFd < 0 || IsFinished()) return 0;

    FD_ZERO(readfds);
    FD_ZERO(writefds);

    switch (m_State) {
        case STATE_CONNECTING:
        case STATE_SENDING:
            FD_SET(m_SocketFd, writefds);
            break;
        case STATE_TLS_HANDSHAKE:
            if (m_SSL) {
                int sslErr = SSL_get_error((SSL*)m_SSL, 0);
                if (sslErr == SSL_ERROR_WANT_WRITE) {
                    FD_SET(m_SocketFd, writefds);
                } else {
                    FD_SET(m_SocketFd, readfds);
                }
            } else {
                FD_SET(m_SocketFd, writefds);
            }
            break;
        case STATE_RECV_HEADERS:
        case STATE_RECV_BODY:
            FD_SET(m_SocketFd, readfds);
            break;
        default:
            return 0;
    }

    return m_SocketFd + 1;
}

void HttpRequest::SetError(const char* context) {
    fprintf(stderr, "HttpRequest error: %s (state=%d)\n", context, m_State);
    CloseSSL();
    CloseSocket();
    m_State = STATE_ERROR;
}

void HttpRequest::CloseSocket() {
    if (m_SocketFd >= 0) {
        CloseSocket(m_SocketFd);
        m_SocketFd = -1;
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
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
    return err;
}

bool HttpRequest::DoConnect() {
    int err = GetSocketError(m_SocketFd);
    if (err != 0) {
        CloseSocket();
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
        return DoTlsHandshake();
    }

    m_SendBuffer = BuildRequest();
    m_SendOffset = 0;
    m_ResponseHeaders.clear();
    m_ResponseBody.clear();
    m_State = STATE_SENDING;
    return DoSend();
}