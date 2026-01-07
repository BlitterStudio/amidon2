#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// Standard POSIX headers for clib2
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <curl/curl.h>
#include <proto/exec.h> // For Library* definition if needed, or void*

// Global references
extern struct Library *SocketBase;
extern struct Library *__SocketBase;

extern "C" long CreateRawAmigaSocket(long domain, long type, long protocol);

// Debug callback for socket creation
curl_socket_t opensocket_callback(void *clientp, curlsocktype purpose, struct curl_sockaddr *address)
{
    // printf(">>> TEST: opensocket_callback invoked. Purpose: %d\n", purpose);
    
    // Check curl_socket_t size
    // printf(">>> TEST: sizeof(curl_socket_t) = %d\n", sizeof(curl_socket_t));

    // Create RAW AMIGA SOCKET
    // We pass protocol 0 just to be safe, though address->protocol should be fine if it's 0 or 6
    // Note: clib2 headers define socket families/types. AmigaOS headers might use same values (BSD standard).
    long raw_sock = CreateRawAmigaSocket(address->family, address->socktype, 0); // Force Proto 0
    
    if (raw_sock < 0) {
        printf(">>> TEST ERROR: CreateRawAmigaSocket failed!\n");
        return CURL_SOCKET_BAD;
    }
    
    printf(">>> TEST: Created RAW Amiga Socket: 0x%lx (%ld)\n", raw_sock, raw_sock);
    
    // DO NOT call standard connect/fcntl on this! It is NOT a clib2 FD.
    // Return directly to libcurl.
    
    return (curl_socket_t)raw_sock;
}

int sockopt_callback(void *clientp, curl_socket_t curlfd, curlsocktype purpose)
{
    // printf(">>> TEST: sockopt_callback invoked. FD: %d\n", curlfd);
    return CURL_SOCKOPT_OK;
}

void TestCurlConnection() {
    printf(">>> TEST: Starting direct libcurl test...\n");
    
    curl_version_info_data *data = curl_version_info(CURLVERSION_NOW);
    printf(">>> TEST: libcurl version: %s\n", data->version);
    printf(">>> TEST: SSL Version: %s\n", data->ssl_version);
    printf(">>> TEST: Protocols:\n");
    const char *const *proto;
    for(proto = data->protocols; *proto; ++proto) {
        printf("  %s\n", *proto);
    }
    
    CURL* curl = curl_easy_init();
    if(curl) {
        // Setup debug callbacks
        curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, opensocket_callback);
        curl_easy_setopt(curl, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);
        
        // TEST 1: HTTP (Plaintext)
        printf(">>> TEST: Attempting HTTP (port 80) connection to example.com...\n");
        curl_easy_setopt(curl, CURLOPT_URL, "http://example.com");
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        // curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        
        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
             printf(">>> TEST ERROR: HTTP curl_easy_perform() failed: %s (Code: %d)\n", curl_easy_strerror(res), res);
        } else {
             printf(">>> TEST SUCCESS: HTTP curl request succeeded!\n");
        }

        // TEST 2: HTTPS (SSL)
        printf(">>> TEST: Attempting HTTPS (port 443) connection to mastodon.social...\n");
        curl_easy_setopt(curl, CURLOPT_URL, "https://mastodon.social");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
             printf(">>> TEST ERROR: HTTPS curl_easy_perform() failed: %s (Code: %d)\n", curl_easy_strerror(res), res);
        } else {
             printf(">>> TEST SUCCESS: HTTPS curl request succeeded!\n");
        }
        
        curl_easy_cleanup(curl);
    } else {
         printf(">>> TEST ERROR: curl_easy_init() failed!\n");
    }
}

void TestSocketConnection() {
    printf("\n\n=======================================================\n");
    printf(">>> TEST: TestSocketConnection() STARTING\n");
    printf("=======================================================\n");
    printf(">>> TEST: Starting raw socket test to mastodon.social:80...\n");
    
    if (!SocketBase) {
        printf(">>> TEST ERROR: SocketBase is NULL!\n");
        return;
    }

    // 1. Get Host by Name
    struct hostent *he = gethostbyname("mastodon.social");
    if (!he) {
        printf(">>> TEST ERROR: gethostbyname failed!\n");
        return;
    }
    printf(">>> TEST: Host resolved. IP: %d.%d.%d.%d\n", 
           (unsigned char)he->h_addr[0], (unsigned char)he->h_addr[1],
           (unsigned char)he->h_addr[2], (unsigned char)he->h_addr[3]);

    // 2. Create Socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf(">>> TEST ERROR: socket() failed!\n");
        return;
    }
    printf(">>> TEST: Socket created: %d\n", sock);

    // 3. Set Non-Blocking (Replicating libcurl behavior)
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
         printf(">>> TEST ERROR: TestSocket - fcntl(F_GETFL) failed. Errno: %d\n", errno);
    } else {
         if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
             printf(">>> TEST ERROR: TestSocket - fcntl(F_SETFL, O_NONBLOCK) failed. Errno: %d\n", errno);
         } else {
             printf(">>> TEST: TestSocket - Enabled O_NONBLOCK.\n");
         }
    }

    // 4. Connect
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(80);
    server.sin_addr = *((struct in_addr *)he->h_addr);
    
    printf(">>> TEST: Calling connect() with O_NONBLOCK...\n");
    int res = connect(sock, (struct sockaddr *)&server, sizeof(server));
    
    if (res < 0) {
        int err = errno;
        printf(">>> TEST: connect() returned -1. Errno: %d (%s)\n", err, strerror(err));
        
        // EINPROGRESS is expected for non-blocking connect
        if (err == EINPROGRESS) {
            printf(">>> TEST: EINPROGRESS received (Correct). Waiting for connection...\n");
            
            // Wait with select
            fd_set myset;
            struct timeval tv;
            FD_ZERO(&myset);
            FD_SET(sock, &myset);
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            
            res = select(sock + 1, NULL, &myset, NULL, &tv);
            if (res > 0) {
                int so_error;
                socklen_t len = sizeof(so_error);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
                if (so_error == 0) {
                    printf(">>> TEST SUCCESS: Connected (async)!\n");
                    // Send simple HEAD request
                    const char* req = "HEAD / HTTP/1.0\r\nHost: mastodon.social\r\n\r\n";
                    send(sock, (const void*)req, strlen(req), 0);
                    char buf[128];
                    int len = recv(sock, buf, sizeof(buf)-1, 0);
                    if (len > 0) {
                        buf[len] = 0;
                        printf(">>> TEST: Received %d bytes: %.20s...\n", len, buf);
                    }
                } else {
                     printf(">>> TEST ERROR: Async connect failed with SO_ERROR: %d\n", so_error);
                }
            } else {
                 printf(">>> TEST ERROR: select() timed out or failed. Res: %d\n", res);
            }
        } else {
            printf(">>> TEST ERROR: Expected EINPROGRESS but got %d!\n", err);
        }
    } else {
        printf(">>> TEST SUCCESS: Connect succeeded immediately (unexpected for non-block)?\n");
    }
    
    close(sock);
}
