/*
 * Amidon2 - a Mastodon client for AmigaOS
 * Copyright (C) 2026 Dimitris Panokostas
 */

#include <cstdio>
#include <proto/exec.h>
#include <proto/dos.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "App.h"

int main(int argc, char** argv) {
    setbuf(stdout, NULL); // Disable buffering
    

    // Verify basic library opening
    {
        struct Library* testBase = OpenLibrary("intuition.library", 36);
        if (testBase) {
            printf("Main: intuition.library v36+ opened successfully.\n");
            CloseLibrary(testBase);
        } else {
            printf("Main: FATAL ERROR - Cannot open intuition.library v36!\n");
        }
    }

    // Run the application   
    {
        printf("Starting Network Test...\n");
        HttpClient client;
        client.Get("https://mastodon.social/api/v2/instance", [](const std::string& response, int code) {
            printf("Network Test Result: Code %d\n", code);
            if (code == 200) {
                printf("Response (first 100 chars): %.100s\n", response.c_str());
            } else {
                printf("Response: %s\n", response.c_str());
            }
        });

        App app;
        app.Run();
    }


    return 0;
}
