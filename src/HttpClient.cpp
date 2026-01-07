#include "HttpClient.h"
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>

// Implementation that shells out to the 'curl' binary.
// This is often more reliable on AmigaOS than linking against libcurl.

namespace {
    struct CurlResult {
        std::string body;
        int code;
    };

    CurlResult ExecuteCurl(const std::string& args) {
        const char* bodyFile = "T:amidon_curl_body";
        const char* codeFile = "T:amidon_curl_code";
        
        // Remove old files just in case
        remove(bodyFile);
        remove(codeFile);

        // Build command. We use --silent to avoid progress bar, --location to follow redirects.
        // We use --header to specify JSON content type for API calls.
        std::string cmd = "curl --silent --location " + args + " --output " + bodyFile + " --write-out \"%{http_code}\" >" + codeFile;
        
        // printf("DEBUG: Executing: %s\n", cmd.c_str());
        int ret = system(cmd.c_str());
        
        CurlResult res = {"", 0};
        
        if (ret == 0) {
            // Read HTTP code
            FILE* fCode = fopen(codeFile, "r");
            if (fCode) {
                if (fscanf(fCode, "%d", &res.code) != 1) res.code = 0;
                fclose(fCode);
            }
            
            // Read Body
            FILE* fBody = fopen(bodyFile, "r");
            if (fBody) {
                fseek(fBody, 0, SEEK_END);
                long size = ftell(fBody);
                fseek(fBody, 0, SEEK_SET);
                if (size > 0) {
                    res.body.resize(size);
                    fread(&res.body[0], 1, size, fBody);
                }
                fclose(fBody);
            }
        } else {
            fprintf(stderr, "ERROR: system() call to curl failed with return: %d\n", ret);
        }
        
        // Cleanup temp files
        remove(bodyFile);
        remove(codeFile);
        
        return res;
    }
}

size_t HttpClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    if (userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
    }
    return size * nmemb;
}

HttpClient::HttpClient() {
}

HttpClient::~HttpClient() {
}

void HttpClient::Get(const std::string& url, HttpCallback callback) {
    std::string args = "\"" + url + "\"";
    CurlResult res = ExecuteCurl(args);
    if (callback) {
        callback(res.body, res.code);
    }
}

void HttpClient::Post(const std::string& url, const std::string& payload, HttpCallback callback) {
    const char* payloadFile = "T:amidon_curl_payload";
    
    // Write payload to temp file to avoid command line length limits/escaping issues
    FILE* fPay = fopen(payloadFile, "wb");
    if (fPay) {
        fwrite(payload.c_str(), 1, payload.length(), fPay);
        fclose(fPay);
    }

    std::string args = "--header \"Content-Type: application/json\" --data @" + std::string(payloadFile) + " \"" + url + "\"";
    CurlResult res = ExecuteCurl(args);
    
    remove(payloadFile);

    if (callback) {
        callback(res.body, res.code);
    }
}



