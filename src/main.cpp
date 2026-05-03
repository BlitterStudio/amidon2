/*
 * Amidon2 - a Mastodon client for AmigaOS
 * Copyright (C) 2026 Dimitris Panokostas
 */

#include <cstdio>

extern "C" {
#include <proto/exec.h>
int __stack_size = 80000;
}

#include "App.h"

int main(int argc, char** argv) {
    setbuf(stdout, NULL);

    App app;
    app.Run();

    return 0;
}
