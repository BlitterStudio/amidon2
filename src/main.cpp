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

#include "App.h"


// Important: SocketBase must be visible for inline calls
struct Library *SocketBase = NULL;
struct Library *__SocketBase = NULL;

// Defined in autoinit_amissl_main.c (modified to be manual)
// Stubs provided here to fix the build as requested.
extern "C" void InitAmiSSL_Manual() {
    // TODO: Implement manual AmiSSL initialization if needed
}
extern "C" void CleanupAmiSSL_Manual() {
    // TODO: Implement manual AmiSSL cleanup if needed
}


int main(int argc, char** argv) {
    setbuf(stdout, NULL); // Disable buffering
    
    // Initialize AmiSSL manually to avoid static initializer crashes
    InitAmiSSL_Manual();

    // Run the application   
    {
        App app;
        app.Run();
    }

    // Cleanup AmiSSL
    CleanupAmiSSL_Manual();

    return 0;
}
