/*
 * Amidon2 - a Mastodon client for AmigaOS
 * Copyright (C) 2026 Dimitris Panokostas
 */

#include <cstdio>

extern "C" {
#include <proto/exec.h>
#include <proto/dos.h>
int __stack_size = 262144;
}

#include "App.h"

int main(int argc, char** argv) {
    setbuf(stdout, NULL);

    {
        App app;
        app.Run();
    }  // ~App runs here — clean shutdown of MUI / HTTP / libraries.

    // Skip clib2's _exit cleanup. clib2's stdio / heap teardown crashes on
    // this cross-toolchain (-fno-exceptions, -fno-rtti, m68k); AmigaOS
    // reclaims memory and library refs on process exit, so the cleanup is
    // unnecessary. ~App above already disposes everything we own.
    Exit(0);
    return 0;  // unreachable
}
