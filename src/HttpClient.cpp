#include "HttpClient.h"

extern "C" {
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/socket.h>
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

const int MAX_REDIRECTS = 5;

struct HttpResult {
    std::string body;
    long statusCode;
};

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

    size_t n = (size_t)(p - hs);
    if (n == 0) return false;
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

    if (*p == 0) {
        path = "/";
    } else {
        path = p;
    }

    return true;
}

std::string GrabHeader(const char* hdrBlock, const char* name) {
    std::string upper = name;
    for (size_t i = 0; i < upper.size(); i++) {
        if (upper[i] == '-') upper[i] = '-';
        else upper[i] = (i == 0 || upper[i-1] == '-') ? (char)toupper((unsigned char)upper[i]) : (char)tolower((unsigned char)upper[i]);
    }
    std::string lower = name;
    for (size_t i = 0; i < lower.size(); i++) lower[i] = (char)tolower((unsigned char)lower[i]);

    std::string prefix1 = upper + ": ";
    std::string prefix2 = lower + ": ";

    const char* v = strstr(hdrBlock, prefix1.c_str());
    if (!v) v = strstr(hdrBlock, prefix2.c_str());
    if (v) {
        v += prefix1.size();
        const char* e = v;
        while (*e && *e != '\r' && *e != '\n') e++;
        return std::string(v, (size_t)(e - v));
    }
    return "";
}

const char* const DefaultCABundles[] = {
    "AmiSSL:Certs/curl-ca-bundle.crt",
    "ENVARC:AmiSSL/Certs/curl-ca-bundle.crt",
    "ENV:AmiSSL/Certs/curl-ca-bundle.crt",
    "SYS:Storage/AmiSSL/Certs/curl-ca-bundle.crt",
    NULL
};

const char* FindCABundle() {
    for (int i = 0; DefaultCABundles[i]; i++) {
        BPTR f = Open((STRPTR)DefaultCABundles[i], MODE_OLDFILE);
        if (f) {
            Close(f);
            return DefaultCABundles[i];
        }
    }
    return NULL;
}

#define SET_AMISSL_BASES(asBase, amsBase) \
    do { AmiSSLBase = (asBase); AmiSSLMasterBase = (amsBase); } while(0)

}

HttpClient::HttpClient() {
}

HttpClient::~HttpClient() {
}

void HttpClient::Get(const std::string& url, HttpCallback callback) {
    PerformRequest("GET", url, "", "", "", callback);
}

void HttpClient::Post(const std::string& url, const std::string& payload, HttpCallback callback) {
    PerformRequest("POST", url, payload, "", "application/json", callback);
}

void HttpClient::GetWithHeaders(const std::string& url, const std::string& authHeader, HttpCallback callback) {
    PerformRequest("GET", url, "", authHeader, "", callback);
}

void HttpClient::PostWithHeaders(const std::string& url, const std::string& payload,
                                 const std::string& contentType, const std::string& authHeader,
                                 HttpCallback callback) {
    PerformRequest("POST", url, payload, authHeader, contentType, callback);
}

void HttpClient::PerformRequest(const std::string& method, const std::string& url,
                                const std::string& body, const std::string& authHeader,
                                const std::string& contentType, HttpCallback callback) {
    HttpResult result;
    result.statusCode = 0;

    // AmiSSL base pointers — set locally for inline macro resolution
    struct Library* AmiSSLBase = NULL;
    struct Library* AmiSSLMasterBase = NULL;

    std::string currentUrl = url;

    for (int redirect = 0; redirect < MAX_REDIRECTS; redirect++) {
        std::string host, path;
        unsigned long port;
        bool isHttps;

        if (!ParseUrl(currentUrl, host, path, port, isHttps)) {
            fprintf(stderr, "HttpClient: unparseable URL '%s'\n", currentUrl.c_str());
            result.statusCode = 0;
            result.body = "Invalid URL";
            break;
        }

        struct Library* SocketBase = OpenLibrary((STRPTR)"bsdsocket.library", 4);
        if (!SocketBase) {
            fprintf(stderr, "HttpClient: failed to open bsdsocket.library\n");
            result.statusCode = 0;
            result.body = "Cannot open bsdsocket.library";
            break;
        }

        struct Library* savedASBase = NULL;
        struct Library* savedAMSBase = NULL;
        SSL_CTX* sslCtx = NULL;
        SSL* ssl = NULL;
        int sslInitialized = 0;
        int sock = -1;

        struct hostent* he = gethostbyname((STRPTR)host.c_str());
        if (!he) {
            fprintf(stderr, "HttpClient: DNS lookup failed for '%s'\n", host.c_str());
            CloseLibrary(SocketBase);
            result.statusCode = 0;
            result.body = "DNS lookup failed";
            break;
        }

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            fprintf(stderr, "HttpClient: socket() failed\n");
            CloseLibrary(SocketBase);
            result.statusCode = 0;
            result.body = "socket() failed";
            break;
        }

        struct sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons((UWORD)port);
        sin.sin_addr.s_addr = *((ULONG*)he->h_addr_list[0]);

        if (connect(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
            fprintf(stderr, "HttpClient: connect() failed for '%s:%lu'\n", host.c_str(), port);
            CloseSocket(sock);
            CloseLibrary(SocketBase);
            result.statusCode = 0;
            result.body = "Connection failed";
            break;
        }

        int useTls = 0;

        if (isHttps) {
            savedAMSBase = OpenLibrary((STRPTR)"amisslmaster.library", AMISSLMASTER_MIN_VERSION);
            if (!savedAMSBase) {
                fprintf(stderr, "HttpClient: failed to open amisslmaster.library\n");
                CloseSocket(sock);
                CloseLibrary(SocketBase);
                result.statusCode = 0;
                result.body = "Cannot open amisslmaster.library";
                break;
            }

            {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                if (!InitAmiSSLMaster(AMISSL_CURRENT_VERSION, TRUE)) {
                    fprintf(stderr, "HttpClient: InitAmiSSLMaster failed\n");
                    CloseLibrary(savedAMSBase);
                    CloseSocket(sock);
                    CloseLibrary(SocketBase);
                    result.statusCode = 0;
                    result.body = "InitAmiSSLMaster failed";
                    break;
                }
            }

            {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                savedASBase = OpenAmiSSL();
            }
            if (!savedASBase) {
                fprintf(stderr, "HttpClient: OpenAmiSSL failed\n");
                CloseLibrary(savedAMSBase);
                CloseSocket(sock);
                CloseLibrary(SocketBase);
                result.statusCode = 0;
                result.body = "OpenAmiSSL failed";
                break;
            }

            int sniErrno = 0;
            {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                struct TagItem initTags[] = {
                    { AmiSSL_ErrNoPtr, (ULONG)&sniErrno },
                    { AmiSSL_SocketBase, (ULONG)SocketBase },
                    { TAG_DONE, 0 }
                };
                int initResult = InitAmiSSLA(initTags);
                if (initResult != 0) {
                    fprintf(stderr, "HttpClient: InitAmiSSLA failed\n");
                    SET_AMISSL_BASES(savedASBase, savedAMSBase);
                    CloseAmiSSL();
                    CloseLibrary(savedAMSBase);
                    CloseSocket(sock);
                    CloseLibrary(SocketBase);
                    result.statusCode = 0;
                    result.body = "InitAmiSSLA failed";
                    break;
                }
            }
            sslInitialized = 1;

            {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                OPENSSL_init_ssl(OPENSSL_INIT_SSL_DEFAULT
                                 | OPENSSL_INIT_ADD_ALL_CIPHERS
                                 | OPENSSL_INIT_ADD_ALL_DIGESTS, NULL);
            }

            {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                unsigned char seed[32];
                ULONG t = (ULONG)FindTask(NULL);
                for (ULONG i = 0; i < sizeof(seed); i++)
                    seed[i] = (unsigned char)(t >> ((i & 3) * 8)) ^ (unsigned char)i;
                RAND_seed(seed, sizeof(seed));
            }

            {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                sslCtx = SSL_CTX_new(TLS_client_method());
            }
            if (!sslCtx) {
                fprintf(stderr, "HttpClient: SSL_CTX_new failed\n");
                if (sslInitialized) {
                    SET_AMISSL_BASES(savedASBase, savedAMSBase);
                    CleanupAmiSSLA(NULL);
                }
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                CloseAmiSSL();
                CloseLibrary(savedAMSBase);
                CloseSocket(sock);
                CloseLibrary(SocketBase);
                result.statusCode = 0;
                result.body = "SSL_CTX_new failed";
                break;
            }

            const char* caBundle = FindCABundle();
            {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                if (caBundle) {
                    if (SSL_CTX_load_verify_locations(sslCtx, caBundle, NULL) != 1) {
                        fprintf(stderr, "HttpClient: CA bundle load failed from %s, continuing without verify\n", caBundle);
                        SSL_CTX_set_verify(sslCtx, SSL_VERIFY_NONE, NULL);
                    } else {
                        SSL_CTX_set_verify(sslCtx, SSL_VERIFY_PEER, NULL);
                    }
                } else {
                    fprintf(stderr, "HttpClient: no CA bundle found, continuing without verification\n");
                    SSL_CTX_set_verify(sslCtx, SSL_VERIFY_NONE, NULL);
                }
            }

            {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                ssl = SSL_new(sslCtx);
            }
            if (!ssl) {
                fprintf(stderr, "HttpClient: SSL_new failed\n");
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                SSL_CTX_free(sslCtx);
                if (sslInitialized) {
                    SET_AMISSL_BASES(savedASBase, savedAMSBase);
                    CleanupAmiSSLA(NULL);
                }
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                CloseAmiSSL();
                CloseLibrary(savedAMSBase);
                CloseSocket(sock);
                CloseLibrary(SocketBase);
                result.statusCode = 0;
                result.body = "SSL_new failed";
                break;
            }

            {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                SSL_set_fd(ssl, sock);
                SSL_set_tlsext_host_name(ssl, host.c_str());
            }

            if (caBundle) {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                SSL_set_hostflags(ssl, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
                SSL_set1_host(ssl, host.c_str());
            }

            {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                int rc = SSL_connect(ssl);
                if (rc <= 0) {
                    int err = SSL_get_error(ssl, rc);
                    long verr = SSL_get_verify_result(ssl);
                    if (verr != X509_V_OK) {
                        fprintf(stderr, "HttpClient: SSL_connect failed rc=%d err=%d verify=%ld (%s)\n",
                                rc, err, verr, X509_verify_cert_error_string(verr));
                    } else {
                        fprintf(stderr, "HttpClient: SSL_connect failed rc=%d err=%d\n", rc, err);
                    }
                    SSL_free(ssl); SSL_CTX_free(sslCtx);
                    if (sslInitialized) CleanupAmiSSLA(NULL);
                    CloseAmiSSL();
                    CloseLibrary(savedAMSBase);
                    CloseSocket(sock); CloseLibrary(SocketBase);
                    result.statusCode = 0;
                    result.body = "TLS handshake failed";
                    break;
                }
            }

            useTls = 1;
        }

        std::string request = method + " " + path + " HTTP/1.1\r\n";
        request += "Host: " + host + "\r\n";
        request += "User-Agent: Amidon/0.2\r\n";
        request += "Accept: */*\r\n";
        request += "Connection: close\r\n";

        if (!authHeader.empty()) {
            request += authHeader + "\r\n";
        }

        if (method == "POST" && !body.empty()) {
            char lenBuf[32];
            snprintf(lenBuf, sizeof(lenBuf), "%lu", (unsigned long)body.size());
            request += "Content-Type: " + (contentType.empty() ? std::string("application/json") : contentType) + "\r\n";
            request += "Content-Length: ";
            request += lenBuf;
            request += "\r\n";
        }

        request += "\r\n";
        if (method == "POST" && !body.empty()) {
            request += body;
        }

        {
            int sendResult = 0;
            if (useTls && ssl) {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                int sent = 0;
                while (sent < (int)request.size()) {
                    int n = SSL_write(ssl, request.c_str() + sent, (int)request.size() - sent);
                    if (n <= 0) { sendResult = -1; break; }
                    sent += n;
                }
            } else {
                int sent = 0;
                while (sent < (int)request.size()) {
                    int n = send(sock, (char*)request.c_str() + sent, (int)request.size() - sent, 0);
                    if (n <= 0) { sendResult = -1; break; }
                    sent += n;
                }
            }

            if (sendResult < 0) {
                fprintf(stderr, "HttpClient: send() failed\n");
                if (useTls) {
                    SET_AMISSL_BASES(savedASBase, savedAMSBase);
                    SSL_shutdown(ssl); SSL_free(ssl); SSL_CTX_free(sslCtx);
                    if (sslInitialized) CleanupAmiSSLA(NULL);
                    CloseAmiSSL();
                }
                CloseSocket(sock); CloseLibrary(SocketBase);
                result.statusCode = 0;
                result.body = "Send failed";
                break;
            }
        }

        std::vector<char> hdrBuf(16384);
        int hdrLen = 0;
        char* hdrEnd = NULL;

        while (hdrLen < (int)hdrBuf.size() - 1) {
            int got;
            if (useTls && ssl) {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                got = SSL_read(ssl, &hdrBuf[hdrLen], (int)hdrBuf.size() - 1 - hdrLen);
            } else {
                got = recv(sock, &hdrBuf[hdrLen], (int)hdrBuf.size() - 1 - hdrLen, 0);
            }
            if (got <= 0) break;
            hdrLen += got;
            hdrBuf[hdrLen] = 0;
            hdrEnd = strstr(&hdrBuf[0], "\r\n\r\n");
            if (hdrEnd) break;
            hdrEnd = strstr(&hdrBuf[0], "\n\n");
            if (hdrEnd) break;
        }

        if (!hdrEnd) {
            fprintf(stderr, "HttpClient: no header terminator in response\n");
            if (useTls) {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                SSL_shutdown(ssl); SSL_free(ssl); SSL_CTX_free(sslCtx);
                if (sslInitialized) CleanupAmiSSLA(NULL);
                CloseAmiSSL();
            }
            CloseSocket(sock); CloseLibrary(SocketBase);
            result.statusCode = 0;
            result.body = "No response headers";
            break;
        }

        int headerBlockSize = (*hdrEnd == '\r')
            ? (int)(hdrEnd - &hdrBuf[0]) + 4
            : (int)(hdrEnd - &hdrBuf[0]) + 2;
        *hdrEnd = 0;

        int code = 0;
        const char* sp = strchr(&hdrBuf[0], ' ');
        if (sp) {
            while (*sp == ' ') sp++;
            while (*sp >= '0' && *sp <= '9') { code = code * 10 + (*sp - '0'); sp++; }
        }

        if ((code == 301 || code == 302 || code == 303 || code == 307 || code == 308)) {
            std::string location = GrabHeader(&hdrBuf[0], "Location");
            if (!location.empty()) {
                if (useTls) {
                    SET_AMISSL_BASES(savedASBase, savedAMSBase);
                    SSL_shutdown(ssl); SSL_free(ssl); SSL_CTX_free(sslCtx);
                    if (sslInitialized) CleanupAmiSSLA(NULL);
                    CloseAmiSSL();
                }
                CloseSocket(sock); CloseLibrary(SocketBase);
                currentUrl = location;
                continue;
            }
        }

        bool chunked = false;
        if (strstr(&hdrBuf[0], "Transfer-Encoding: chunked") ||
            strstr(&hdrBuf[0], "transfer-encoding: chunked")) {
            chunked = true;
        }

        std::string responseBody;
        int bodyAlready = hdrLen - headerBlockSize;
        if (bodyAlready > 0) {
            responseBody.append(&hdrBuf[0] + headerBlockSize, bodyAlready);
        }

        if (chunked) {
            while (true) {
                char line[32];
                int ll;
                if (useTls && ssl) {
                    SET_AMISSL_BASES(savedASBase, savedAMSBase);
                    int i = 0;
                    while (i < (int)sizeof(line) - 1) {
                        char c;
                        int g = SSL_read(ssl, &c, 1);
                        if (g <= 0) { i = -1; break; }
                        if (c == '\n') { line[i] = 0; break; }
                        if (c != '\r') line[i++] = c;
                    }
                    ll = i;
                } else {
                    int i = 0;
                    while (i < (int)sizeof(line) - 1) {
                        char c;
                        int g = recv(sock, &c, 1, 0);
                        if (g <= 0) { i = -1; break; }
                        if (c == '\n') { line[i] = 0; break; }
                        if (c != '\r') line[i++] = c;
                    }
                    ll = i;
                }
                if (ll < 0) break;
                if (line[0] == 0) {
                    if (useTls && ssl) {
                        SET_AMISSL_BASES(savedASBase, savedAMSBase);
                        int i = 0;
                        while (i < (int)sizeof(line) - 1) {
                            char c;
                            int g = SSL_read(ssl, &c, 1);
                            if (g <= 0) { i = -1; break; }
                            if (c == '\n') { line[i] = 0; break; }
                            if (c != '\r') line[i++] = c;
                        }
                        ll = i;
                    } else {
                        int i = 0;
                        while (i < (int)sizeof(line) - 1) {
                            char c;
                            int g = recv(sock, &c, 1, 0);
                            if (g <= 0) { i = -1; break; }
                            if (c == '\n') { line[i] = 0; break; }
                            if (c != '\r') line[i++] = c;
                        }
                        ll = i;
                    }
                    if (ll < 0) break;
                }
                unsigned long chunkSize = strtoul(line, NULL, 16);
                if (chunkSize == 0) break;

                unsigned long have = 0;
                while (have < chunkSize) {
                    char tmpBuf[4096];
                    unsigned long want = chunkSize - have;
                    if (want > sizeof(tmpBuf)) want = sizeof(tmpBuf);
                    int got;
                    if (useTls && ssl) {
                        SET_AMISSL_BASES(savedASBase, savedAMSBase);
                        got = SSL_read(ssl, tmpBuf, (int)want);
                    } else {
                        got = recv(sock, tmpBuf, (int)want, 0);
                    }
                    if (got <= 0) goto body_done;
                    responseBody.append(tmpBuf, got);
                    have += (unsigned long)got;
                }
                if (useTls && ssl) {
                    SET_AMISSL_BASES(savedASBase, savedAMSBase);
                    while (true) {
                        char c;
                        int g = SSL_read(ssl, &c, 1);
                        if (g <= 0) break;
                        if (c == '\n') break;
                    }
                } else {
                    while (true) {
                        char c;
                        int g = recv(sock, &c, 1, 0);
                        if (g <= 0) break;
                        if (c == '\n') break;
                    }
                }
            }
        } else {
            std::string contentLenStr = GrabHeader(&hdrBuf[0], "Content-Length");
            if (!contentLenStr.empty()) {
                unsigned long contentLen = strtoul(contentLenStr.c_str(), NULL, 10);
                while ((unsigned long)responseBody.size() < contentLen) {
                    char tmpBuf[4096];
                    unsigned long want = contentLen - (unsigned long)responseBody.size();
                    if (want > sizeof(tmpBuf)) want = sizeof(tmpBuf);
                    int got;
                    if (useTls && ssl) {
                        SET_AMISSL_BASES(savedASBase, savedAMSBase);
                        got = SSL_read(ssl, tmpBuf, (int)want);
                    } else {
                        got = recv(sock, tmpBuf, (int)want, 0);
                    }
                    if (got <= 0) break;
                    responseBody.append(tmpBuf, got);
                }
            } else {
                while (true) {
                    char tmpBuf[4096];
                    int got;
                    if (useTls && ssl) {
                        SET_AMISSL_BASES(savedASBase, savedAMSBase);
                        got = SSL_read(ssl, tmpBuf, sizeof(tmpBuf));
                    } else {
                        got = recv(sock, tmpBuf, sizeof(tmpBuf), 0);
                    }
                    if (got <= 0) break;
                    responseBody.append(tmpBuf, got);
                }
            }
        }

body_done:
        result.body = responseBody;
        result.statusCode = code;

        if (useTls) {
            SET_AMISSL_BASES(savedASBase, savedAMSBase);
            SSL_shutdown(ssl); SSL_free(ssl); SSL_CTX_free(sslCtx);
            if (sslInitialized) CleanupAmiSSLA(NULL);
            CloseAmiSSL();
        }
        CloseSocket(sock);
        CloseLibrary(SocketBase);
        break;
    }

    if (callback) {
        callback(result.body, result.statusCode);
    }
}
