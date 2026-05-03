#ifndef CACHE_MANAGER_H
#define CACHE_MANAGER_H

#include <string>
#include "logic/MastodonTypes.h"

class CacheManager {
public:
    CacheManager();
    
    // Directory management
    static bool CreateCacheDirectory(const std::string& instance);
    static bool CacheExists(const std::string& instance);
    static std::string GetCachePath(const std::string& instance);
    
    // Credentials management
    static bool SaveCreds(const std::string& instance, const std::string& client_id, const std::string& client_secret);
    static bool LoadCreds(const std::string& instance, AppRegistration& reg);
    
    // Token management
    static bool SaveToken(const std::string& instance, const std::string& access_token);
    static bool LoadToken(const std::string& instance, std::string& access_token);
    
    // Avatar management
    static bool SaveAvatar(const std::string& instance, const std::string& avatarUrl);
    static std::string GetAvatarPath(const std::string& instance);
    
private:
    static std::string GetCredsPath(const std::string& instance);
    static std::string GetTokenPath(const std::string& instance);
};

#endif // CACHE_MANAGER_H
