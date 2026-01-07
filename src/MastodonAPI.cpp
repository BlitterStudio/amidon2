#include "MastodonAPI.h"
#include <cstdio>

MastodonAPI::MastodonAPI() {
}

void MastodonAPI::RegisterApp(const std::string& instance, std::function<void(bool success, const AppRegistration& reg)> callback) {
    std::string url = "https://" + instance + "/api/v1/apps";
    
    // Construct simple JSON payload manually to avoid heavy dependencies for sending
    // Since we only send simple fields, this is safe enough.
    std::string payload = "{";
    payload += "\"client_name\": \"Amidon\",";
    payload += "\"redirect_uris\": \"urn:ietf:wg:oauth:2.0:oob\",";
    payload += "\"scopes\": \"read write follow push\"";
    payload += "}";

    m_Client.Post(url, payload, [callback, instance](const std::string& response, int code) {
        if (code == 200) {
            picojson::value v;
            std::string err = picojson::parse(v, response);
            if (!err.empty()) {
                fprintf(stderr, "JSON parse error: %s\n", err.c_str());
                if (callback) callback(false, {});
                return;
            }

            if (!v.is<picojson::object>()) {
                fprintf(stderr, "JSON is not an object\n");
                 if (callback) callback(false, {});
                 return;
            }
            
            picojson::object& o = v.get<picojson::object>();
            AppRegistration reg;
            reg.instance_url = instance;
            if (o["client_id"].is<std::string>()) reg.client_id = o["client_id"].get<std::string>();
            if (o["client_secret"].is<std::string>()) reg.client_secret = o["client_secret"].get<std::string>();

            if (callback) callback(true, reg);
        } else {
            fprintf(stderr, "RegisterApp failed with code: %d\n", code);
            if (callback) callback(false, {});
        }
    });
}

void MastodonAPI::GetToken(const AppRegistration& reg, const std::string& authCode, std::function<void(bool success, const std::string& token)> callback) {
    std::string url = "https://" + reg.instance_url + "/oauth/token";

    std::string payload = "{";
    payload += "\"client_id\": \"" + reg.client_id + "\",";
    payload += "\"client_secret\": \"" + reg.client_secret + "\",";
    payload += "\"redirect_uri\": \"urn:ietf:wg:oauth:2.0:oob\",";
    payload += "\"grant_type\": \"authorization_code\",";
    payload += "\"code\": \"" + authCode + "\"";
    payload += "}";
    
    m_Client.Post(url, payload, [callback](const std::string& response, int code) {
        if (code == 200) {
             picojson::value v;
             std::string err = picojson::parse(v, response);
             if (!err.empty()) {
                 fprintf(stderr, "JSON parse error: %s\n", err.c_str());
                 if (callback) callback(false, "");
                 return;
             }
             
             if (!v.is<picojson::object>()) {
                  if (callback) callback(false, "");
                  return;
             }

             picojson::object& o = v.get<picojson::object>();
             if (o["access_token"].is<std::string>()) {
                 std::string token = o["access_token"].get<std::string>();
                 if (callback) callback(true, token);
             } else {
                 if (callback) callback(false, "");
             }
        } else {
            fprintf(stderr, "GetToken failed with code: %d\n", code);
            if (callback) callback(false, "");
        }
    });
}

void MastodonAPI::SetCredentials(const std::string& instance, const std::string& token) {
    m_InstanceURL = instance;
    m_AccessToken = token;
}

void MastodonAPI::GetHomeTimeline(std::function<void(std::vector<Status>)> callback) {
    if (m_InstanceURL.empty() || m_AccessToken.empty()) {
        // printf("DEBUG: GetHomeTimeline skipped - Not Authenticated (URL: '%s', Token empty? %d)\n", m_InstanceURL.c_str(), m_AccessToken.empty());
        if (callback) callback({});
        return;
    }

    std::string url = "https://" + m_InstanceURL + "/api/v1/timelines/home?access_token=" + m_AccessToken;
    // printf("DEBUG: GetHomeTimeline URL: %s\n", url.c_str());
    
    m_Client.Get(url, [callback](const std::string& response, int code) {
        // printf("DEBUG: API Response Code: %d\n", code);
        // printf("DEBUG: API Response Body: %s\n", response.c_str()); // Commented out to avoid spam
        
        if (code == 200) {
            picojson::value v;
            std::string err = picojson::parse(v, response);
            if (!err.empty()) {
                 fprintf(stderr, "JSON parse error: %s\n", err.c_str());
                 if (callback) callback({});
                 return;
            }
            
            if (!v.is<picojson::array>()) {
                fprintf(stderr, "JSON is not an array\n");
                if (callback) callback({});
                return;
            }
            
            std::vector<Status> timeline;
            const picojson::array& list = v.get<picojson::array>();
            
            for (const auto& item : list) {
                if (item.is<picojson::object>()) {
                    const picojson::object& statusObj = item.get<picojson::object>();
                    Status s;
                    
                    if (statusObj.count("id") && statusObj.at("id").is<std::string>())
                         s.id = statusObj.at("id").get<std::string>();
                         
                    if (statusObj.count("content") && statusObj.at("content").is<std::string>())
                         s.content = statusObj.at("content").get<std::string>();
                    
                    // Account
                    if (statusObj.count("account") && statusObj.at("account").is<picojson::object>()) {
                        const picojson::object& acct = statusObj.at("account").get<picojson::object>();
                        if (acct.count("username") && acct.at("username").is<std::string>())
                            s.author_username = acct.at("username").get<std::string>();
                        if (acct.count("display_name") && acct.at("display_name").is<std::string>())
                            s.author_displayname = acct.at("display_name").get<std::string>();
                    }
                    
                    timeline.push_back(s);
                }
            }
            
            if (callback) callback(timeline);
        } else {
             fprintf(stderr, "GetHomeTimeline failed with code: %d\n", code);
             if (callback) callback({});
        }
    });
}


bool MastodonAPI::HasCredentials() const {
    return !m_InstanceURL.empty() && !m_AccessToken.empty();
}

void MastodonAPI::GetPublicTimeline(std::function<void(std::vector<Status>)> callback) {
    if (m_InstanceURL.empty()) {
        if (callback) callback({});
        return;
    }

    // Public timeline does not strictly require a token
    std::string url = "https://" + m_InstanceURL + "/api/v1/timelines/public?local=true"; // Default to local public
    // printf("DEBUG: GetPublicTimeline URL: %s\n", url.c_str());
    
    m_Client.Get(url, [callback](const std::string& response, int code) {
        // printf("DEBUG: API Response Code: %d\n", code);
        
        if (code == 200) {
            picojson::value v;
            std::string err = picojson::parse(v, response);
            if (!err.empty()) {
                 fprintf(stderr, "JSON parse error: %s\n", err.c_str());
                 if (callback) callback({});
                 return;
            }
            
            if (!v.is<picojson::array>()) {
                fprintf(stderr, "JSON is not an array\n");
                if (callback) callback({});
                return;
            }
            
            std::vector<Status> timeline;
            const picojson::array& list = v.get<picojson::array>();
            
            for (const auto& item : list) {
                if (item.is<picojson::object>()) {
                    const picojson::object& statusObj = item.get<picojson::object>();
                    Status s;
                    
                    if (statusObj.count("id") && statusObj.at("id").is<std::string>())
                         s.id = statusObj.at("id").get<std::string>();
                         
                    if (statusObj.count("content") && statusObj.at("content").is<std::string>())
                         s.content = statusObj.at("content").get<std::string>();
                    
                    // Account
                    if (statusObj.count("account") && statusObj.at("account").is<picojson::object>()) {
                        const picojson::object& acct = statusObj.at("account").get<picojson::object>();
                        if (acct.count("username") && acct.at("username").is<std::string>())
                            s.author_username = acct.at("username").get<std::string>();
                        if (acct.count("display_name") && acct.at("display_name").is<std::string>())
                            s.author_displayname = acct.at("display_name").get<std::string>();
                    }
                    
                    timeline.push_back(s);
                }
            }
            
            if (callback) callback(timeline);
        } else {
             fprintf(stderr, "GetPublicTimeline failed with code: %d\n", code);
             if (callback) callback({});
        }
    });
}

void MastodonAPI::GetNotifications(std::function<void(std::vector<std::string>)> callback) {
    if (callback) callback({});
}

bool MastodonAPI::Login(const std::string& instance, const std::string& email, const std::string& password) {
    return false;
}
