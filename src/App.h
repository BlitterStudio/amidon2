/*
 * Amidon - a Mastodon client for AmigaOS
 * Copyright (C) 2024 Dimitris Panokostas
 */

#ifndef APP_H
#define APP_H

#include <libraries/mui.h>
#include <string>
#include <memory>

class MainWindow;
class WelcomeDialog; // Forward declaration
class SettingsDialog; // Forward declaration
class LoginDialog; // Forward declaration
#include "../MastodonAPI.h" // Include API definition

class App {
public:
    App();
    ~App();

    void Run();

private:
   std::unique_ptr<MastodonAPI> m_API;
   std::unique_ptr<AppRegistration> m_AppRegistration;
   
   // ... rest of private members
    Object* m_App;
    
    // Windows
    std::unique_ptr<MainWindow> m_MainWindow;
    // We'll use raw pointers for MUI objects if they are simple, 
    // but better to wrap them in classes for logic handling.
    // For now, let's keep references to active dialog helpers if needed,
    // or just let them manage their own MUI objects.
    
    // Actually, following the pattern of MainWindow, we should probably have
    // member variables for the dialog instances.
    std::unique_ptr<WelcomeDialog> m_WelcomeDialog;
    std::unique_ptr<SettingsDialog> m_SettingsDialog;
    std::unique_ptr<LoginDialog> m_LoginDialog;

    bool m_Running;
    bool m_OwnedSocketBase = false; // Track if we opened SocketBase
    
    void InitializeMUI();
    void CleanupMUI();
    
    // Logic
    bool HasSettings(); 
    void ShowWelcome();
    void ShowMain();
    void ShowSettings();
    void ShowLogin();
};

#include "Constants.h"

#endif // APP_H
