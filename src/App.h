/*
 * Amidon2 - a Mastodon client for AmigaOS
 * Copyright (C) 2026 Dimitris Panokostas
 */

#ifndef APP_H
#define APP_H

#include <libraries/mui.h>
#include <string>
#include <memory>
#include <functional>

#include "MastodonAPI.h"
#include "AsyncHttpService.h"

class MainWindow;
class WelcomeDialog;
class SettingsDialog;
class LoginDialog;

class App {
public:
    App();
    ~App();

    void Run();

private:
    std::unique_ptr<MastodonAPI> m_API;
    std::unique_ptr<AppRegistration> m_AppRegistration;
    std::unique_ptr<AsyncHttpService> m_AsyncHttp;

    Object* m_App;

    std::unique_ptr<MainWindow> m_MainWindow;
    std::unique_ptr<WelcomeDialog> m_WelcomeDialog;
    std::unique_ptr<SettingsDialog> m_SettingsDialog;
    std::unique_ptr<LoginDialog> m_LoginDialog;

    bool m_Running;
    bool m_OwnedSocketBase = false;

    void InitializeMUI();
    void CleanupMUI();

    bool HasSettings();
    void ShowWelcome();
    void ShowMain();
    void ShowSettings();
    void ShowLogin();

    // Async avatar download. Fires callback with the local file path on
    // success, or an empty string on failure. No-op (callback fires with
    // empty path) if AsyncHttp isn't available or the URL is empty.
    void DownloadAvatar(const std::string& instance, const std::string& url,
                        std::function<void(const std::string& path)> done);
};

#include "Constants.h"

#endif // APP_H
