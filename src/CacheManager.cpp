#include "CacheManager.h"
#include "HttpClient.h"
#include "logic/CredsParser.h"

extern "C" {
#include <proto/dos.h>
}

#include <json-c/json.h>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

CacheManager::CacheManager() {
}

std::string CacheManager::GetCachePath(const std::string& instance) {
    return "cache/" + instance;
}

std::string CacheManager::GetCredsPath(const std::string& instance) {
    return GetCachePath(instance) + "/creds";
}

std::string CacheManager::GetTokenPath(const std::string& instance) {
    return GetCachePath(instance) + "/token";
}

std::string CacheManager::GetAvatarPath(const std::string& instance) {
    return GetCachePath(instance) + "/avatar.png";
}

bool CacheManager::CreateCacheDirectory(const std::string& instance) {
    if (instance.empty()) {
        fprintf(stderr, "ERROR: Cannot create cache directory - instance is empty\n");
        return false;
    }

    struct stat st;
    if (stat("cache", &st) != 0) {
#ifdef __amigaos__
        BPTR lock = CreateDir("cache");
        if (!lock) {
            fprintf(stderr, "ERROR: Failed to create cache directory\n");
            return false;
        }
        UnLock(lock);
#else
        if (mkdir("cache", 0755) != 0) {
            fprintf(stderr, "ERROR: Failed to create cache directory\n");
            return false;
        }
#endif
    }

    std::string cachePath = GetCachePath(instance);
    if (stat(cachePath.c_str(), &st) != 0) {
#ifdef __amigaos__
        BPTR lock = CreateDir(cachePath.c_str());
        if (!lock) {
            fprintf(stderr, "ERROR: Failed to create cache directory for instance: %s\n", instance.c_str());
            return false;
        }
        UnLock(lock);
#else
        if (mkdir(cachePath.c_str(), 0755) != 0) {
            fprintf(stderr, "ERROR: Failed to create cache directory for instance: %s\n", instance.c_str());
            return false;
        }
#endif
    }

    printf("Cache directory created/verified: %s\n", cachePath.c_str());
    return true;
}

bool CacheManager::CacheExists(const std::string& instance) {
    if (instance.empty()) return false;

    std::string cachePath = GetCachePath(instance);
    struct stat st;
    return (stat(cachePath.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}

bool CacheManager::SaveCreds(const std::string& instance, const std::string& client_id, const std::string& client_secret) {
    if (!CreateCacheDirectory(instance)) {
        return false;
    }

    std::string credsPath = GetCredsPath(instance);

    json_object* obj = json_object_new_object();
    json_object_object_add(obj, "client_id", json_object_new_string(client_id.c_str()));
    json_object_object_add(obj, "client_secret", json_object_new_string(client_secret.c_str()));
    const char* json = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY);

    FILE* f = fopen(credsPath.c_str(), "w");
    if (!f) {
        fprintf(stderr, "ERROR: Failed to open creds file for writing: %s\n", credsPath.c_str());
        json_object_put(obj);
        return false;
    }

    fwrite(json, 1, strlen(json), f);
    fclose(f);
    json_object_put(obj);

    printf("Credentials saved to: %s\n", credsPath.c_str());
    return true;
}

bool CacheManager::LoadCreds(const std::string& instance, AppRegistration& reg) {
    std::string credsPath = GetCredsPath(instance);

    FILE* f = fopen(credsPath.c_str(), "r");
    if (!f) {
        return false;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::string content;
    content.resize(fsize);
    fread(&content[0], 1, fsize, f);
    fclose(f);

    if (!CredsParser::ParseCreds(content, instance, reg)) {
        fprintf(stderr, "ERROR: Failed to parse creds JSON\n");
        return false;
    }

    printf("Credentials loaded from: %s\n", credsPath.c_str());
    return true;
}

bool CacheManager::SaveToken(const std::string& instance, const std::string& access_token) {
    if (!CreateCacheDirectory(instance)) {
        return false;
    }

    std::string tokenPath = GetTokenPath(instance);

    json_object* obj = json_object_new_object();
    json_object_object_add(obj, "access_token", json_object_new_string(access_token.c_str()));
    json_object_object_add(obj, "token_type", json_object_new_string("Bearer"));
    json_object_object_add(obj, "scope", json_object_new_string("read write follow push"));
    const char* json = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY);

    FILE* f = fopen(tokenPath.c_str(), "w");
    if (!f) {
        fprintf(stderr, "ERROR: Failed to open token file for writing: %s\n", tokenPath.c_str());
        json_object_put(obj);
        return false;
    }

    fwrite(json, 1, strlen(json), f);
    fclose(f);
    json_object_put(obj);

    printf("Token saved to: %s\n", tokenPath.c_str());
    return true;
}

bool CacheManager::LoadToken(const std::string& instance, std::string& access_token) {
    std::string tokenPath = GetTokenPath(instance);

    FILE* f = fopen(tokenPath.c_str(), "r");
    if (!f) {
        fprintf(stderr, "WARNING: Token file not found: %s\n", tokenPath.c_str());
        return false;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::string content;
    content.resize(fsize);
    size_t readSize = fread(&content[0], 1, fsize, f);
    fclose(f);
    content.resize(readSize);

    if (content.empty()) {
        fprintf(stderr, "ERROR: Token file is empty: %s\n", tokenPath.c_str());
        return false;
    }

    json_object* root = json_tokener_parse(content.c_str());
    if (CredsParser::ParseToken(content, access_token)) {
        printf("Token loaded from: %s\n", tokenPath.c_str());
        return true;
    }

    // Fallback: treat entire file contents as a bare token string (old Amidon compat)
    std::string token = content;
    while (!token.empty() && (token.back() == '\n' || token.back() == '\r' || token.back() == ' '))
        token.pop_back();
    if (!token.empty()) {
        access_token = token;
        printf("Token loaded (plain text) from: %s\n", tokenPath.c_str());
        return true;
    }

    fprintf(stderr, "ERROR: Token file has no usable token: %s\n", tokenPath.c_str());
    return false;
}

bool CacheManager::SaveAvatar(const std::string& instance, const std::string& avatarUrl) {
    if (avatarUrl.empty()) {
        fprintf(stderr, "WARNING: Avatar URL is empty, skipping download\n");
        return false;
    }

    if (!CreateCacheDirectory(instance)) {
        return false;
    }

    std::string avatarPath = GetAvatarPath(instance);

    printf("Downloading avatar from: %s\n", avatarUrl.c_str());
    printf("Saving to: %s\n", avatarPath.c_str());

    HttpClient client;
    bool success = false;

    client.Get(avatarUrl, [&avatarPath, &success](const std::string& response, long code) {
        if (code == 200) {
            FILE* f = fopen(avatarPath.c_str(), "wb");
            if (f) {
                fwrite(response.c_str(), 1, response.length(), f);
                fclose(f);
                printf("Avatar saved successfully\n");
                success = true;
            } else {
                fprintf(stderr, "ERROR: Failed to open avatar file for writing: %s\n", avatarPath.c_str());
            }
        } else {
            fprintf(stderr, "ERROR: Failed to download avatar, HTTP code: %ld\n", code);
        }
    });

    return success;
}
