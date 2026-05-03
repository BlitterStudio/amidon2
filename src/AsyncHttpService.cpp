#include "AsyncHttpService.h"

extern "C" {
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dostags.h>
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

extern struct Library* __SocketBase;

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

typedef struct {
    char* data;
    unsigned long size;
    unsigned long cap;
} CBuffer;

static void CBufInit(CBuffer* b) {
    b->data = NULL;
    b->size = 0;
    b->cap = 0;
}

static int CBufAppend(CBuffer* b, const char* d, unsigned long n) {
    if (b->size + n + 1 > b->cap) {
        unsigned long nc = b->cap ? b->cap * 2 : 16384;
        while (nc < b->size + n + 1) nc *= 2;
        char* nd = (char*)AllocVec(nc, MEMF_ANY);
        if (!nd) return 0;
        if (b->data) {
            CopyMem(b->data, nd, b->size);
            FreeVec(b->data);
        }
        b->data = nd;
        b->cap = nc;
    }
    CopyMem(d, b->data + b->size, n);
    b->size += n;
    b->data[b->size] = '\0';
    return 1;
}

static void CBufFree(CBuffer* b) {
    if (b->data) {
        FreeVec(b->data);
        b->data = NULL;
    }
    b->size = 0;
    b->cap = 0;
}

static char* CBufDetach(CBuffer* b) {
    char* d = b->data;
    b->data = NULL;
    b->size = 0;
    b->cap = 0;
    return d;
}

static int ParseUrlC(const char* url, char* host, int hostSz, char* path, int pathSz,
                      unsigned long* port, int* isHttps) {
    const char* p = url;
    unsigned long defaultPort = 80;
    *isHttps = 0;

    if (strncmp(p, "https://", 8) == 0) {
        p += 8;
        defaultPort = 443;
        *isHttps = 1;
    } else if (strncmp(p, "http://", 7) == 0) {
        p += 7;
    } else {
        return FALSE;
    }

    const char* hs = p;
    while (*p && *p != '/' && *p != ':') p++;

    int n = (int)(p - hs);
    if (n == 0 || n >= hostSz) return FALSE;
    CopyMem(hs, host, n);
    host[n] = '\0';

    *port = defaultPort;
    if (*p == ':') {
        p++;
        *port = 0;
        while (*p >= '0' && *p <= '9') {
            *port = *port * 10 + (*p - '0');
            p++;
        }
        if (*port == 0 || *port > 65535) return FALSE;
    }

    if (*p == '\0') {
        path[0] = '/'; path[1] = '\0';
    } else {
        int plen = strlen(p);
        if (plen >= pathSz) return FALSE;
        strcpy(path, p);
    }

    return TRUE;
}

static const char* FindHeaderValueC(const char* hdr, const char* name) {
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

static const char* FindCABundleC(void) {
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

static int PerformHttpInChild(const char* method, const char* url, const char* body,
                               const char* authHeader, const char* contentType,
                               char** outResponse, unsigned long* outResponseLen,
                               long* outCode) {
    struct Library* AmiSSLBase = NULL;
    struct Library* AmiSSLMasterBase = NULL;

    CBuffer response;
    CBufInit(&response);
    *outCode = 0;

    char currentUrl[2048];
    strncpy(currentUrl, url, sizeof(currentUrl) - 1);
    currentUrl[sizeof(currentUrl) - 1] = '\0';

    for (int redirect = 0; redirect < MAX_REDIRECTS; redirect++) {
        char host[256], path[2048];
        unsigned long port;
        int isHttps;

        if (!ParseUrlC(currentUrl, host, (int)sizeof(host), path, (int)sizeof(path), &port, &isHttps)) {
            *outCode = 0;
            CBufFree(&response);
            return FALSE;
        }

        if (!__SocketBase) {
            __SocketBase = OpenLibrary((STRPTR)"bsdsocket.library", 4);
            if (!__SocketBase) {
                __SocketBase = OpenLibrary((STRPTR)"bsdsocket.library", 0);
            }
        }
        if (!__SocketBase) {
            *outCode = 0;
            CBufFree(&response);
            return FALSE;
        }

        struct hostent* he = gethostbyname((STRPTR)host);
        if (!he) {
            *outCode = 0;
            CBufFree(&response);
            return FALSE;
        }

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            *outCode = 0;
            CBufFree(&response);
            return FALSE;
        }

        struct sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons((UWORD)port);
        sin.sin_addr.s_addr = *((ULONG*)he->h_addr_list[0]);

        if (connect(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
            CloseSocket(sock);
            *outCode = 0;
            CBufFree(&response);
            return FALSE;
        }

        int useTls = 0;
        struct Library* savedASBase = NULL;
        struct Library* savedAMSBase = NULL;
        SSL_CTX* sslCtx = NULL;
        SSL* ssl = NULL;
        int sslInitialized = 0;

        if (isHttps) {
            savedAMSBase = OpenLibrary((STRPTR)"amisslmaster.library", AMISSLMASTER_MIN_VERSION);
            if (!savedAMSBase) {
                CloseSocket(sock);
                *outCode = 0;
                CBufFree(&response);
                return FALSE;
            }

            SET_AMISSL_BASES(savedASBase, savedAMSBase);
            if (!InitAmiSSLMaster(AMISSL_CURRENT_VERSION, TRUE)) {
                CloseLibrary(savedAMSBase);
                CloseSocket(sock);
                *outCode = 0;
                CBufFree(&response);
                return FALSE;
            }

            SET_AMISSL_BASES(savedASBase, savedAMSBase);
            savedASBase = OpenAmiSSL();
            if (!savedASBase) {
                CloseLibrary(savedAMSBase);
                CloseSocket(sock);
                *outCode = 0;
                CBufFree(&response);
                return FALSE;
            }

            int sniErrno = 0;
            SET_AMISSL_BASES(savedASBase, savedAMSBase);
            struct TagItem initTags[] = {
                { AmiSSL_ErrNoPtr, (ULONG)&sniErrno },
                { AmiSSL_SocketBase, (ULONG)__SocketBase },
                { TAG_DONE, 0 }
            };
            if (InitAmiSSLA(initTags) != 0) {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                CloseAmiSSL();
                CloseLibrary(savedAMSBase);
                CloseSocket(sock);
                *outCode = 0;
                CBufFree(&response);
                return FALSE;
            }
            sslInitialized = 1;

            SET_AMISSL_BASES(savedASBase, savedAMSBase);
            OPENSSL_init_ssl(OPENSSL_INIT_SSL_DEFAULT
                             | OPENSSL_INIT_ADD_ALL_CIPHERS
                             | OPENSSL_INIT_ADD_ALL_DIGESTS, NULL);

            SET_AMISSL_BASES(savedASBase, savedAMSBase);
            {
                unsigned char seed[32];
                ULONG t = (ULONG)FindTask(NULL);
                for (ULONG i = 0; i < sizeof(seed); i++)
                    seed[i] = (unsigned char)(t >> ((i & 3) * 8)) ^ (unsigned char)i;
                RAND_seed(seed, sizeof(seed));
            }

            SET_AMISSL_BASES(savedASBase, savedAMSBase);
            sslCtx = SSL_CTX_new(TLS_client_method());
            if (!sslCtx) {
                if (sslInitialized) {
                    SET_AMISSL_BASES(savedASBase, savedAMSBase);
                    CleanupAmiSSLA(NULL);
                }
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                CloseAmiSSL();
                CloseLibrary(savedAMSBase);
                CloseSocket(sock);
                *outCode = 0;
                CBufFree(&response);
                return FALSE;
            }

            const char* caBundle = FindCABundleC();
            SET_AMISSL_BASES(savedASBase, savedAMSBase);
            if (caBundle) {
                if (SSL_CTX_load_verify_locations(sslCtx, caBundle, NULL) != 1) {
                    SSL_CTX_set_verify(sslCtx, SSL_VERIFY_NONE, NULL);
                } else {
                    SSL_CTX_set_verify(sslCtx, SSL_VERIFY_PEER, NULL);
                }
            } else {
                SSL_CTX_set_verify(sslCtx, SSL_VERIFY_NONE, NULL);
            }

            SET_AMISSL_BASES(savedASBase, savedAMSBase);
            ssl = SSL_new(sslCtx);
            if (!ssl) {
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
                *outCode = 0;
                CBufFree(&response);
                return FALSE;
            }

            SET_AMISSL_BASES(savedASBase, savedAMSBase);
            SSL_set_fd(ssl, sock);
            SSL_set_tlsext_host_name(ssl, host);

            if (caBundle) {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                SSL_set_hostflags(ssl, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
                SSL_set1_host(ssl, host);
            }

            SET_AMISSL_BASES(savedASBase, savedAMSBase);
            int rc = SSL_connect(ssl);
            if (rc <= 0) {
                SSL_free(ssl); SSL_CTX_free(sslCtx);
                if (sslInitialized) CleanupAmiSSLA(NULL);
                CloseAmiSSL();
                CloseLibrary(savedAMSBase);
                CloseSocket(sock);
                *outCode = 0;
                CBufFree(&response);
                return FALSE;
            }

            useTls = 1;
        }

        char reqBuf[4096];
        int reqLen = 0;

        reqLen += snprintf(reqBuf + reqLen, sizeof(reqBuf) - reqLen,
                           "%s %s HTTP/1.1\r\n", method, path);
        reqLen += snprintf(reqBuf + reqLen, sizeof(reqBuf) - reqLen,
                           "Host: %s\r\n", host);
        reqLen += snprintf(reqBuf + reqLen, sizeof(reqBuf) - reqLen,
                           "User-Agent: Amidon/0.2\r\n");
        reqLen += snprintf(reqBuf + reqLen, sizeof(reqBuf) - reqLen,
                           "Accept: */*\r\n");
        reqLen += snprintf(reqBuf + reqLen, sizeof(reqBuf) - reqLen,
                           "Connection: close\r\n");

        if (authHeader && authHeader[0]) {
            reqLen += snprintf(reqBuf + reqLen, sizeof(reqBuf) - reqLen,
                               "%s\r\n", authHeader);
        }

        unsigned long bodyLen = body ? strlen(body) : 0;
        if (strcmp(method, "POST") == 0 && bodyLen > 0) {
            const char* ct = (contentType && contentType[0]) ? contentType : "application/json";
            reqLen += snprintf(reqBuf + reqLen, sizeof(reqBuf) - reqLen,
                               "Content-Type: %s\r\n", ct);
            reqLen += snprintf(reqBuf + reqLen, sizeof(reqBuf) - reqLen,
                               "Content-Length: %lu\r\n", bodyLen);
        }

        reqLen += snprintf(reqBuf + reqLen, sizeof(reqBuf) - reqLen, "\r\n");

        {
            int sendOk = 1;
            int sent = 0;
            while (sent < reqLen) {
                int n;
                if (useTls && ssl) {
                    SET_AMISSL_BASES(savedASBase, savedAMSBase);
                    n = SSL_write(ssl, reqBuf + sent, reqLen - sent);
                } else {
                    n = send(sock, reqBuf + sent, reqLen - sent, 0);
                }
                if (n <= 0) { sendOk = 0; break; }
                sent += n;
            }

            if (bodyLen > 0 && sendOk) {
                sent = 0;
                while (sent < (int)bodyLen) {
                    int n;
                    if (useTls && ssl) {
                        SET_AMISSL_BASES(savedASBase, savedAMSBase);
                        n = SSL_write(ssl, body + sent, (int)bodyLen - sent);
                    } else {
                        n = send(sock, (char*)(body + sent), (int)bodyLen - sent, 0);
                    }
                    if (n <= 0) { sendOk = 0; break; }
                    sent += n;
                }
            }

            if (!sendOk) {
                if (useTls) {
                    SET_AMISSL_BASES(savedASBase, savedAMSBase);
                    SSL_shutdown(ssl); SSL_free(ssl); SSL_CTX_free(sslCtx);
                    if (sslInitialized) CleanupAmiSSLA(NULL);
                    CloseAmiSSL();
                    CloseLibrary(savedAMSBase);
                }
                CloseSocket(sock);
                *outCode = 0;
                CBufFree(&response);
                return FALSE;
            }
        }

        char hdrBuf[16384];
        int hdrLen = 0;
        char* hdrEnd = NULL;

        while (hdrLen < (int)sizeof(hdrBuf) - 1) {
            int got;
            if (useTls && ssl) {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                got = SSL_read(ssl, hdrBuf + hdrLen, (int)sizeof(hdrBuf) - 1 - hdrLen);
            } else {
                got = recv(sock, hdrBuf + hdrLen, (int)sizeof(hdrBuf) - 1 - hdrLen, 0);
            }
            if (got <= 0) break;
            hdrLen += got;
            hdrBuf[hdrLen] = '\0';
            hdrEnd = strstr(hdrBuf, "\r\n\r\n");
            if (hdrEnd) break;
            hdrEnd = strstr(hdrBuf, "\n\n");
            if (hdrEnd) break;
        }

        if (!hdrEnd) {
            if (useTls) {
                SET_AMISSL_BASES(savedASBase, savedAMSBase);
                SSL_shutdown(ssl); SSL_free(ssl); SSL_CTX_free(sslCtx);
                if (sslInitialized) CleanupAmiSSLA(NULL);
                CloseAmiSSL();
                CloseLibrary(savedAMSBase);
            }
            CloseSocket(sock);
            *outCode = 0;
            CBufFree(&response);
            return FALSE;
        }

        int headerBlockSize = (*hdrEnd == '\r')
            ? (int)(hdrEnd - hdrBuf) + 4
            : (int)(hdrEnd - hdrBuf) + 2;
        *hdrEnd = '\0';

        int code = 0;
        const char* sp = strchr(hdrBuf, ' ');
        if (sp) {
            while (*sp == ' ') sp++;
            while (*sp >= '0' && *sp <= '9') { code = code * 10 + (*sp - '0'); sp++; }
        }

        if (code == 301 || code == 302 || code == 303 || code == 307 || code == 308) {
            const char* loc = FindHeaderValueC(hdrBuf, "Location");
            if (loc) {
                if (useTls) {
                    SET_AMISSL_BASES(savedASBase, savedAMSBase);
                    SSL_shutdown(ssl); SSL_free(ssl); SSL_CTX_free(sslCtx);
                    if (sslInitialized) CleanupAmiSSLA(NULL);
                    CloseAmiSSL();
                    CloseLibrary(savedAMSBase);
                }
                CloseSocket(sock);

                int locLen = strlen(loc);
                const char* locEnd = loc;
                while (*locEnd && *locEnd != '\r' && *locEnd != '\n') locEnd++;
                int copyLen = (int)(locEnd - loc);
                if (copyLen >= (int)sizeof(currentUrl)) copyLen = (int)sizeof(currentUrl) - 1;
                CopyMem(loc, currentUrl, copyLen);
                currentUrl[copyLen] = '\0';

                continue;
            }
        }

        int chunked = 0;
        if (strstr(hdrBuf, "Transfer-Encoding: chunked") ||
            strstr(hdrBuf, "transfer-encoding: chunked")) {
            chunked = 1;
        }

        int bodyAlready = hdrLen - headerBlockSize;
        if (bodyAlready > 0) {
            if (!CBufAppend(&response, hdrBuf + headerBlockSize, bodyAlready)) {
                if (useTls) {
                    SET_AMISSL_BASES(savedASBase, savedAMSBase);
                    SSL_shutdown(ssl); SSL_free(ssl); SSL_CTX_free(sslCtx);
                    if (sslInitialized) CleanupAmiSSLA(NULL);
                    CloseAmiSSL();
                    CloseLibrary(savedAMSBase);
                }
                CloseSocket(sock);
                *outCode = 0;
                CBufFree(&response);
                return FALSE;
            }
        }

        if (chunked) {
            while (TRUE) {
                char line[32];
                int ll;
                {
                    int i = 0;
                    while (i < (int)sizeof(line) - 1) {
                        char c;
                        int g;
                        if (useTls && ssl) {
                            SET_AMISSL_BASES(savedASBase, savedAMSBase);
                            g = SSL_read(ssl, &c, 1);
                        } else {
                            g = recv(sock, &c, 1, 0);
                        }
                        if (g <= 0) { i = -1; break; }
                        if (c == '\n') { line[i] = '\0'; break; }
                        if (c != '\r') line[i++] = c;
                    }
                    ll = i;
                }
                if (ll < 0) break;
                if (line[0] == '\0') {
                    int i = 0;
                    while (i < (int)sizeof(line) - 1) {
                        char c;
                        int g;
                        if (useTls && ssl) {
                            SET_AMISSL_BASES(savedASBase, savedAMSBase);
                            g = SSL_read(ssl, &c, 1);
                        } else {
                            g = recv(sock, &c, 1, 0);
                        }
                        if (g <= 0) { i = -1; break; }
                        if (c == '\n') { line[i] = '\0'; break; }
                        if (c != '\r') line[i++] = c;
                    }
                    if (i < 0) break;
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
                    if (!CBufAppend(&response, tmpBuf, (unsigned long)got)) {
                        goto body_done;
                    }
                    have += (unsigned long)got;
                }
                while (TRUE) {
                    char c;
                    int g;
                    if (useTls && ssl) {
                        SET_AMISSL_BASES(savedASBase, savedAMSBase);
                        g = SSL_read(ssl, &c, 1);
                    } else {
                        g = recv(sock, &c, 1, 0);
                    }
                    if (g <= 0) goto body_done;
                    if (c == '\n') break;
                }
            }
        } else {
            const char* clStr = FindHeaderValueC(hdrBuf, "Content-Length");
            if (clStr) {
                const char* clEnd = clStr;
                while (*clEnd && *clEnd != '\r' && *clEnd != '\n') clEnd++;
                char clBuf[32];
                int clCopyLen = (int)(clEnd - clStr);
                if (clCopyLen >= (int)sizeof(clBuf)) clCopyLen = (int)sizeof(clBuf) - 1;
                CopyMem(clStr, clBuf, clCopyLen);
                clBuf[clCopyLen] = '\0';
                unsigned long contentLen = strtoul(clBuf, NULL, 10);

                while (response.size < contentLen) {
                    char tmpBuf[4096];
                    unsigned long want = contentLen - response.size;
                    if (want > sizeof(tmpBuf)) want = sizeof(tmpBuf);
                    int got;
                    if (useTls && ssl) {
                        SET_AMISSL_BASES(savedASBase, savedAMSBase);
                        got = SSL_read(ssl, tmpBuf, (int)want);
                    } else {
                        got = recv(sock, tmpBuf, (int)want, 0);
                    }
                    if (got <= 0) break;
                    if (!CBufAppend(&response, tmpBuf, (unsigned long)got)) break;
                }
            } else {
                while (TRUE) {
                    char tmpBuf[4096];
                    int got;
                    if (useTls && ssl) {
                        SET_AMISSL_BASES(savedASBase, savedAMSBase);
                        got = SSL_read(ssl, tmpBuf, sizeof(tmpBuf));
                    } else {
                        got = recv(sock, tmpBuf, sizeof(tmpBuf), 0);
                    }
                    if (got <= 0) break;
                    if (!CBufAppend(&response, tmpBuf, (unsigned long)got)) break;
                }
            }
        }

body_done:
        *outCode = code;

        if (useTls) {
            SET_AMISSL_BASES(savedASBase, savedAMSBase);
            SSL_shutdown(ssl); SSL_free(ssl); SSL_CTX_free(sslCtx);
            if (sslInitialized) CleanupAmiSSLA(NULL);
            CloseAmiSSL();
            CloseLibrary(savedAMSBase);
        }
        CloseSocket(sock);
        break;
    }

    *outResponse = CBufDetach(&response);
    *outResponseLen = response.size;
    response.size = 0;
    return (*outCode > 0) ? TRUE : FALSE;
}

} // anonymous namespace

static volatile AsyncHttpWork* g_AsyncWork = NULL;

extern "C" void AsyncHttpEntry(void) {
    struct Library* savedSocketBase = __SocketBase;

    __SocketBase = OpenLibrary((STRPTR)"bsdsocket.library", 4);
    if (!__SocketBase) {
        __SocketBase = OpenLibrary((STRPTR)"bsdsocket.library", 0);
    }

    AsyncHttpWork* work = (AsyncHttpWork*)g_AsyncWork;
    if (work && __SocketBase) {
        char* responseBody = NULL;
        unsigned long responseLen = 0;
        long code = 0;

        int ok = PerformHttpInChild(
            work->method, work->url, work->payload,
            work->authHeader, work->contentType,
            &responseBody, &responseLen, &code
        );

        work->responseBody = responseBody;
        work->responseCode = code;
        work->success = ok;
    } else if (work) {
        work->responseBody = NULL;
        work->responseCode = 0;
        work->success = FALSE;
    }

    if (__SocketBase && __SocketBase != savedSocketBase) {
        CloseLibrary(__SocketBase);
    }
    __SocketBase = savedSocketBase;

    if (work) {
        ReplyMsg(&work->msg);
    }
}

AsyncHttpService::AsyncHttpService()
    : m_ReplyPort(nullptr), m_ReplySig(0), m_Busy(false),
      m_CurrentWork(nullptr) {
}

AsyncHttpService::~AsyncHttpService() {
    Shutdown();
}

bool AsyncHttpService::Init() {
    m_ReplyPort = CreateMsgPort();
    if (!m_ReplyPort) {
        return false;
    }
    m_ReplySig = 1UL << m_ReplyPort->mp_SigBit;
    return true;
}

void AsyncHttpService::Shutdown() {
    if (m_Busy && m_ReplyPort) {
        Wait(m_ReplySig);
        HandleCompletion();
    }
    while (!m_Queue.empty()) {
        m_Queue.clear();
    }
    if (m_ReplyPort) {
        DeleteMsgPort(m_ReplyPort);
        m_ReplyPort = nullptr;
        m_ReplySig = 0;
    }
}

ULONG AsyncHttpService::SignalMask() const {
    return m_ReplySig;
}

bool AsyncHttpService::IsBusy() const {
    return m_Busy;
}

void AsyncHttpService::Get(const std::string& url, const std::string& authHeader,
                            HttpCallback callback) {
    StartRequest("GET", url.c_str(), nullptr, authHeader.c_str(), nullptr, callback);
}

void AsyncHttpService::Post(const std::string& url, const std::string& payload,
                             const std::string& authHeader, const std::string& contentType,
                             HttpCallback callback) {
    StartRequest("POST", url.c_str(), payload.c_str(), authHeader.c_str(),
                 contentType.c_str(), callback);
}

bool AsyncHttpService::StartRequest(const char* method, const char* url, const char* payload,
                                     const char* authHeader, const char* contentType,
                                     HttpCallback callback) {
    if (!m_ReplyPort) return false;

    if (m_Busy) {
        QueuedRequest req;
        req.method = method;
        req.url = url;
        req.payload = payload ? payload : "";
        req.authHeader = authHeader ? authHeader : "";
        req.contentType = contentType ? contentType : "";
        req.callback = callback;
        m_Queue.push_back(req);
        return true;
    }

    AsyncHttpWork* work = (AsyncHttpWork*)AllocVec(sizeof(AsyncHttpWork), MEMF_CLEAR | MEMF_PUBLIC);
    if (!work) {
        if (callback) callback("", 0);
        return false;
    }

    work->msg.mn_ReplyPort = m_ReplyPort;
    work->msg.mn_Length = sizeof(AsyncHttpWork);
    work->msg.mn_Node.ln_Type = NT_MESSAGE;

    work->url = (char*)AllocVec(strlen(url) + 1, MEMF_PUBLIC);
    work->method = (char*)AllocVec(strlen(method) + 1, MEMF_PUBLIC);
    work->payload = payload ? (char*)AllocVec(strlen(payload) + 1, MEMF_PUBLIC) : nullptr;
    work->authHeader = authHeader ? (char*)AllocVec(strlen(authHeader) + 1, MEMF_PUBLIC) : nullptr;
    work->contentType = contentType ? (char*)AllocVec(strlen(contentType) + 1, MEMF_PUBLIC) : nullptr;

    if (!work->url || !work->method ||
        (payload && !work->payload) ||
        (authHeader && !work->authHeader) ||
        (contentType && !work->contentType)) {
        if (work->url) FreeVec(work->url);
        if (work->method) FreeVec(work->method);
        if (work->payload) FreeVec(work->payload);
        if (work->authHeader) FreeVec(work->authHeader);
        if (work->contentType) FreeVec(work->contentType);
        FreeVec(work);
        if (callback) callback("", 0);
        return false;
    }

    strcpy(work->url, url);
    strcpy(work->method, method);
    if (work->payload) strcpy(work->payload, payload);
    if (work->authHeader) strcpy(work->authHeader, authHeader);
    if (work->contentType) strcpy(work->contentType, contentType);

    work->responseBody = nullptr;
    work->responseCode = 0;
    work->success = FALSE;
    work->replyPort = m_ReplyPort;

    m_Callback = callback;
    m_CurrentWork = work;
    m_Busy = true;

    g_AsyncWork = work;

    struct Process* child = (struct Process*)CreateNewProcTags(
        NP_Entry, (ULONG)AsyncHttpEntry,
        NP_StackSize, (ULONG)131072,
        NP_Name, (ULONG)"Amidon HTTP Worker",
        NP_Priority, (ULONG)0,
        TAG_DONE
    );

    if (!child) {
        fprintf(stderr, "AsyncHttpService: CreateNewProc failed\n");
        if (work->url) FreeVec(work->url);
        if (work->method) FreeVec(work->method);
        if (work->payload) FreeVec(work->payload);
        if (work->authHeader) FreeVec(work->authHeader);
        if (work->contentType) FreeVec(work->contentType);
        FreeVec(work);
        g_AsyncWork = NULL;
        m_CurrentWork = nullptr;
        m_Busy = false;
        if (callback) callback("", 0);
        return false;
    }

    return true;
}

void AsyncHttpService::HandleCompletion() {
    if (!m_Busy || !m_ReplyPort) return;

    struct Message* msg = GetMsg(m_ReplyPort);
    if (!msg) return;

    AsyncHttpWork* work = (AsyncHttpWork*)msg;

    HttpCallback cb = m_Callback;
    std::string responseBody;
    long responseCode = work->responseCode;

    if (work->success && work->responseBody) {
        responseBody.assign(work->responseBody);
    }

    if (work->responseBody) FreeVec(work->responseBody);
    if (work->url) FreeVec(work->url);
    if (work->method) FreeVec(work->method);
    if (work->payload) FreeVec(work->payload);
    if (work->authHeader) FreeVec(work->authHeader);
    if (work->contentType) FreeVec(work->contentType);
    FreeVec(work);

    g_AsyncWork = NULL;
    m_CurrentWork = nullptr;
    m_Busy = false;

    if (cb) {
        cb(responseBody, responseCode);
    }

    ProcessQueue();
}

void AsyncHttpService::ProcessQueue() {
    if (m_Busy || m_Queue.empty()) return;

    QueuedRequest req = m_Queue.front();
    m_Queue.erase(m_Queue.begin());

    StartRequest(req.method.c_str(), req.url.c_str(),
                 req.payload.empty() ? nullptr : req.payload.c_str(),
                 req.authHeader.empty() ? nullptr : req.authHeader.c_str(),
                 req.contentType.empty() ? nullptr : req.contentType.c_str(),
                 req.callback);
}
