/*
 * Amidon - a Mastodon client for AmigaOS
 * Copyright (C) 2024 Dimitris Panokostas
 */

#include <cstdio>
#include <proto/exec.h>
#include <proto/dos.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// Use standard headers for clib2 compatibility test
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <curl/curl.h>

// Defined in autoinit_amissl_main.c (modified to be manual)
extern "C" void InitAmiSSL_Manual();
extern "C" void CleanupAmiSSL_Manual();

extern "C" unsigned long __stack = 262144; // 256KB Stack

// Important: SocketBase must be visible for inline calls
extern struct Library *SocketBase;

// Global reference for debug
extern struct Library *__SocketBase;

// External test functions defined in NetworkTest.cpp
extern void TestSocketConnection();
extern void TestCurlConnection();

int main(int argc, char** argv) {
    setbuf(stdout, NULL); // Disable buffering
    
    // Initialize AmiSSL manually to avoid static initializer crashes
    InitAmiSSL_Manual();
    
    // Initialize libcurl global
    curl_global_init(CURL_GLOBAL_ALL);
    
    // Run Diagnostics
    // This will now definitely call the function in NetworkTest.cpp
    TestSocketConnection();
    
    // Uncomment to test full libcurl + SSL
    TestCurlConnection();

    // Run the application
    /*
    {
        App app;
        app.Run();
    }
    */

    // Cleanup libcurl
    curl_global_cleanup();
    
    // Cleanup AmiSSL
    CleanupAmiSSL_Manual();

    return 0;
}
