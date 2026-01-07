/*
 * Amidon2 - a Mastodon client for AmigaOS
 * Copyright (C) 2026 Dimitris Panokostas
 */

#include "App.h"
#include "ui/MainWindow.h"
#include "ui/WelcomeDialog.h"
#include "ui/SettingsDialog.h"
#include "ui/LoginDialog.h"

#include "ui/MUIHelpers.h"
extern "C" {
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
}

#include <cstdio>

#include <vector>

// picojson included via MastodonAPI.h or we can include it directly
// #include "picojson.h" 

struct Library *MUIMasterBase;
struct IntuitionBase *IntuitionBase;

App::App() : m_App(nullptr), m_Running(false) {
    // printf("DEBUG: App::App() start (Heap allocated)\n");
    
    // Initialize System/Network Libraries FIRST
    InitializeMUI();
    
    // printf("DEBUG: Creating MastodonAPI...\n");
    m_API = std::make_unique<MastodonAPI>();
    // printf("DEBUG: Creating AppRegistration...\n");
    m_AppRegistration = std::make_unique<AppRegistration>();

    // printf("DEBUG: App members initialized.\n");

    // printf("DEBUG: App constructor completed.\n");
}

 App::~App() {
     // Explicitly destroy API/Curl BEFORE closing network libraries
     m_API.reset();
     m_AppRegistration.reset();

     if (m_App) {
         MUI_DisposeObject(m_App);
     }
     CleanupMUI();
 }
 
// Define global SocketBase (managed by autoinit or manually)
struct Library *SocketBase = NULL;
struct Library *__SocketBase = NULL;
 
 void App::InitializeMUI() {
     // printf("DEBUG: InitializeMUI start\n");
     
     // Open bsdsocket.library. Version 4 is Roadshow/AmiTCP. 
     // We'll try 3 first as it's common for many stacks, or 0 for any.
     if (!SocketBase) {
          SocketBase = OpenLibrary("bsdsocket.library", 3);
          if (!SocketBase) {
              // Try any version as last resort
              SocketBase = OpenLibrary("bsdsocket.library", 0);
          }
          
          if (!SocketBase) {
              printf("FATAL: Failed to open bsdsocket.library (any version)!\n");
          } else {
              m_OwnedSocketBase = true;
          }
     }

     // Lower Intuition requirement to 36 (OS 2.0) as it should be enough for basic MUI
     IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);
     if (!IntuitionBase) {
         printf("FATAL: intuition.library v36+ failed to open!\n");
     }

     // MUI requires 19+ (MUI 3.x) or higher for modern features
     MUIMasterBase = OpenLibrary("muimaster.library", 19);
     if (!MUIMasterBase) {
         printf("FATAL: muimaster.library v19+ failed to open!\n");
     }
 }
 
 void App::CleanupMUI() {
     if (SocketBase && m_OwnedSocketBase) {
         CloseLibrary(SocketBase);
         SocketBase = NULL;
     }
     if (MUIMasterBase) CloseLibrary(MUIMasterBase);
     if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
 }

 
 bool App::HasSettings() {
     FILE* f = fopen("settings.json", "r");
     if (f) {
         fclose(f);
         return true;
     }
     return false;
 }
 
 void App::Run() {
     // printf("DEBUG: App::Run() start\n");
     
     if (!MUIMasterBase) {
         printf("FATAL: MUIMasterBase is NULL, cannot create App object.\n");
         return;
     }
     
     // printf("DEBUG: Creating MUI App object...\n");
     m_App = MUIHelpers_NewObject(MUIC_Application,
         MUIA_Application_Title, (IPTR)"Amidon2",
         MUIA_Application_Version, (IPTR)"$VER: 0.1",
         MUIA_Application_Copyright, (IPTR)"(C) 2026 Dimitris Panokostas",
         MUIA_Application_Author, (IPTR)"Dimitris Panokostas",
         MUIA_Application_Description, (IPTR)"Mastodon Client for AmigaOS",
         MUIA_Application_Base, (IPTR)"AMIDON",
         TAG_DONE);
     
     if (!m_App) {
         printf("FATAL: Failed to create MUI Application object!\n");
         return;
     }
     // printf("DEBUG: MUI App object created: %p\n", m_App);

     // Create and add MainWindow to Application
     // printf("DEBUG: Creating MainWindow...\n");
     m_MainWindow = std::make_unique<MainWindow>(*m_API);
     Object* winObj = m_MainWindow->Create();
     if (!winObj) {
        printf("FATAL: Failed to create Main Window object!\n");
     }
     DoMethod(m_App, OM_ADDMEMBER, (IPTR)winObj);
 
     // Initialize Notifications for MainWindow
     m_MainWindow->InitNotifications(m_App);
     
     // Setup close request for MainWindow
     DoMethod(m_MainWindow->GetMUIObject(), MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
              m_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
 
     // Check settings and decide startup window
     // If settings exist, load them and set API token
     if (HasSettings()) {
         FILE* f = fopen("settings.json", "rb");
         if (f) {
             fseek(f, 0, SEEK_END);
             long fsize = ftell(f);
             fseek(f, 0, SEEK_SET);  /* same as rewind(f); */
             
             std::vector<char> buffer(fsize + 1); // +1 for null terminator if needed, though string handles it
             size_t readSize = fread(buffer.data(), 1, fsize, f);
             fclose(f);
             buffer[readSize] = 0;
 
             std::string jsonStr(buffer.data());
             
             picojson::value v;
             std::string err = picojson::parse(v, jsonStr);
             
             if (err.empty() && v.is<picojson::object>()) {
                 picojson::object& obj = v.get<picojson::object>();
                 
                 std::string instance, token, client_id, client_secret;
                 
                 if (obj["instance"].is<std::string>()) instance = obj["instance"].get<std::string>();
                 if (obj["access_token"].is<std::string>()) token = obj["access_token"].get<std::string>();
                 if (obj["client_id"].is<std::string>()) client_id = obj["client_id"].get<std::string>();
                 if (obj["client_secret"].is<std::string>()) client_secret = obj["client_secret"].get<std::string>();
                 
                 if (!client_id.empty()) {
                     m_AppRegistration->client_id = client_id;
                     m_AppRegistration->client_secret = client_secret;
                     m_AppRegistration->instance_url = instance;
                 }
                 
                 if (!instance.empty() && !token.empty()) {
                     printf("DEBUG: Loaded settings - Instance: %s, Token found.\n", instance.c_str());
                     m_API->SetCredentials(instance, token);
                     
                     ShowMain();
                     m_MainWindow->FetchTimeline();
                 } else {
                     // printf("DEBUG: Settings loaded but missing instance or token. Inst: %s, Token empty? %d\n", instance.c_str(), token.empty());
                     
                     // Allow Main Window if we have at least an instance
                     if (!instance.empty()) {
                         // printf("DEBUG: Instance found, proceeding to Main Window (Unauthenticated)\n");
                         m_API->SetCredentials(instance, ""); // Set instance at least
                         ShowMain();
                         m_MainWindow->FetchTimeline(); 
                     } else {
                         ShowWelcome();
                     }
                 }
             } else {
                 // printf("DEBUG: JSON parse failed or not an object: %s\n", err.c_str());
                 printf("DEBUG: JSON parse failed or not an object: %s\n", err.c_str());
                 ShowWelcome();
             }
         } else {
             // Failed to open file despite HasSettings() returning true?
             ShowWelcome(); 
         }
     } else {
         ShowWelcome();
     }
 
     m_Running = true;
     // Main Loop
     ULONG signals = 0;
     while (m_Running) {
         ULONG id = DoMethod(m_App, MUIM_Application_NewInput, (IPTR)&signals);
         
         switch(id) {
             case (ULONG)MUIV_Application_ReturnID_Quit:
                 m_Running = false;
                 break;
             case APPRETURN_SETTINGS:
                 ShowSettings();
                 break;
             case APPRETURN_LOGIN:
                 ShowLogin();
                 break;
             case APPRETURN_DO_LOGIN:
                 {
                     if (m_LoginDialog) {
                         std::string code = m_LoginDialog->GetAuthCode();
                         if (!code.empty()) {
                             // Exchange code for token
                             printf("Exchanging code: %s for token...\n", code.c_str());
                             
                             m_API->GetToken(*m_AppRegistration, code, [this](bool success, const std::string& token) {
                                 if (success) {
                                     printf("Login Successful! Token: %s\n", token.c_str());
                                     m_API->SetCredentials(m_AppRegistration->instance_url, token);
                                     
                                     // Save to settings
                                     picojson::object j;
                                     j["instance"] = picojson::value(m_AppRegistration->instance_url);
                                     j["client_id"] =  picojson::value(m_AppRegistration->client_id);
                                     j["client_secret"] =  picojson::value(m_AppRegistration->client_secret);
                                     j["access_token"] =  picojson::value(token);
                                     
                                     std::string s = picojson::value(j).serialize(true);
                                     
                                     FILE* f = fopen("settings.json", "w");
                                     if (f) {
                                         fwrite(s.c_str(), 1, s.length(), f);
                                         fclose(f);
                                     }
                                     
                                     // Close Login Dialog
                                     if (m_LoginDialog) {
                                          SetAttrs(m_LoginDialog->GetMUIObject(), MUIA_Window_Open, FALSE, TAG_DONE);
                                     }
                                     
                                     // Refresh Timeline
                                     m_MainWindow->FetchTimeline();
                                     ShowMain();
                                 } else {
                                     printf("Login Failed!\n");
                                     // TODO: Show Error Dialog
                                 }
                             });
                         }
                     }
                 }
                 break;
             case APPRETURN_REFRESH_TIMELINE:
                 printf("DEBUG: APPRETURN_REFRESH_TIMELINE received.\n");
                 if (m_MainWindow) {
                     m_MainWindow->FetchTimeline();
                 }
                 break;
             case APPRETURN_SAVE_SETTINGS:
                 {
                     // Create settings file with actual values
                     if (m_SettingsDialog) {
                          std::string instance = m_SettingsDialog->GetInstanceURL();
                          
                          // Simplified: Just save instance.
                          picojson::object j;
                          j["instance"] = picojson::value(instance);
                          
                          std::string s = picojson::value(j).serialize(true);
                          
                          FILE* f = fopen("settings.json", "w");
                          if (f) {
                              fwrite(s.c_str(), 1, s.length(), f);
                              fclose(f);
                              printf("Settings saved to settings.json\n");
                          } else {
                              printf("ERROR: Failed to open settings.json for writing!\n");
                          }
 
                         SetAttrs(m_SettingsDialog->GetMUIObject(), MUIA_Window_Open, FALSE, TAG_DONE);
                     }
                     
                     // Close welcome dialog if open
                     if (m_WelcomeDialog) {
                         SetAttrs(m_WelcomeDialog->GetMUIObject(), MUIA_Window_Open, FALSE, TAG_DONE);
                     }
                     
                     // Show Main Window
                     ShowMain();
                 }
                 break;
         }
         
         if (m_Running && signals) {
             signals = Wait(signals | SIGBREAKF_CTRL_C);
             if (signals & SIGBREAKF_CTRL_C) break;
         }
     }
 }
 
 void App::ShowMain() {
     if (m_MainWindow) {
         SetAttrs(m_MainWindow->GetMUIObject(), MUIA_Window_Open, TRUE, TAG_DONE);
     }
 }
 
 void App::ShowWelcome() {
     if (!m_WelcomeDialog) {
         m_WelcomeDialog = std::make_unique<WelcomeDialog>();
         // Add to application
         DoMethod(m_App, OM_ADDMEMBER, (IPTR)m_WelcomeDialog->Create());
         m_WelcomeDialog->InitNotifications(m_App);
     }
     SetAttrs(m_WelcomeDialog->GetMUIObject(), MUIA_Window_Open, TRUE, TAG_DONE);
     
     // Wire up "Open Settings" button if possible here, or in the class itself
     // Ideally the class exposes the button object or sends a return ID itself
 }
 
 void App::ShowSettings() {
     if (!m_SettingsDialog) {
         m_SettingsDialog = std::make_unique<SettingsDialog>();
         DoMethod(m_App, OM_ADDMEMBER, (IPTR)m_SettingsDialog->Create());
         m_SettingsDialog->InitNotifications(m_App);
     }
     SetAttrs(m_SettingsDialog->GetMUIObject(), MUIA_Window_Open, TRUE, TAG_DONE);
 }
 
 void App::ShowLogin() {
     if (!m_LoginDialog) {
         m_LoginDialog = std::make_unique<LoginDialog>();
         DoMethod(m_App, OM_ADDMEMBER, (IPTR)m_LoginDialog->Create());
         m_LoginDialog->InitNotifications(m_App);
     }
     SetAttrs(m_LoginDialog->GetMUIObject(), MUIA_Window_Open, TRUE, TAG_DONE);
     
     // Check if we need to register
     // Load instance from settings
     std::string instance;
     if (HasSettings()) {
         FILE* f = fopen("settings.json", "rb");
         if (f) {
             fseek(f, 0, SEEK_END);
             long fsize = ftell(f);
             fseek(f, 0, SEEK_SET);

             std::vector<char> buffer(fsize + 1);
             size_t readSize = fread(buffer.data(), 1, fsize, f);
             fclose(f);
             buffer[readSize] = 0;

             std::string jsonStr(buffer.data());
             
             picojson::value v;
             std::string err = picojson::parse(v, jsonStr);
             if (err.empty() && v.is<picojson::object>()) {
                 picojson::object& obj = v.get<picojson::object>();
                 if (obj["instance"].is<std::string>()) instance = obj["instance"].get<std::string>();
             }
         }
     }
     
     if (instance.empty()) {
         // Show error or ask for settings
         return;
     }
     
     if (m_AppRegistration->client_id.empty()) { 
         // Not registered yet
         m_LoginDialog->SetAuthURL("Registering...");
         m_API->RegisterApp(instance, [this](bool success, const AppRegistration& reg) {
             if (success) {
                 *m_AppRegistration = reg;
                 // Construct Auth URL
                 std::string authUrl = "https://" + reg.instance_url + "/oauth/authorize?client_id=" + reg.client_id + "&response_type=code&redirect_uri=urn:ietf:wg:oauth:2.0:oob&scope=read+write+follow";
                 if (m_LoginDialog) {
                     m_LoginDialog->SetAuthURL(authUrl.c_str());
                 }
                 // Save registration to updated settings
                 // ... (Optional, but good practice so we don't spam register)
             } else {
                 if (m_LoginDialog) {
                     m_LoginDialog->SetAuthURL("Registration Failed!");
                 }
             }
         });
     } else {
         // Already registered, construct URL
         std::string authUrl = "https://" + m_AppRegistration->instance_url + "/oauth/authorize?client_id=" + m_AppRegistration->client_id + "&response_type=code&redirect_uri=urn:ietf:wg:oauth:2.0:oob&scope=read+write+follow";
         m_LoginDialog->SetAuthURL(authUrl.c_str());
     }
 }
