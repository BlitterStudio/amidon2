/*
 * Amidon2 - a Mastodon client for AmigaOS
 * Copyright (C) 2026 Dimitris Panokostas
 */

#include "App.h"
#include "ui/MainWindow.h"
#include "ui/WelcomeDialog.h"
#include "ui/SettingsDialog.h"
#include "ui/LoginDialog.h"
#include "CacheManager.h"
#include "logic/SettingsParser.h"
#include "logic/UrlBuilder.h"

#include "ui/MUIHelpers.h"
#include "FdSetCompat.h"
extern "C" {
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/datatypes.h>
#include <proto/utility.h>
#include <proto/socket.h>
#include <clib/alib_protos.h>
#include <mui/Guigfx_mcc.h>
}

#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <vector>

#include <json-c/json.h>

struct IntuitionBase *IntuitionBase;
struct Library *MUIMasterBase;
struct GfxBase *GfxBase;
struct Library *DataTypesBase;
struct Library *UtilityBase;

App::App() : m_App(nullptr), m_Running(false) {
    InitializeMUI();

    m_API = std::make_unique<MastodonAPI>();
    m_AppRegistration = std::make_unique<AppRegistration>();
}

App::~App() {
    // Cancel any in-flight HTTP request first so HttpRequest's destructor
    // (CloseSSL + CloseSocket) runs while bsdsocket.library is still open.
    m_AsyncHttp.reset();
    m_API.reset();
    m_AppRegistration.reset();

    if (m_App) {
        MUI_DisposeObject(m_App);
        m_App = nullptr;
    }
    CleanupMUI();
}

struct Library *SocketBase = NULL;
struct Library *__SocketBase = NULL;

void App::InitializeMUI() {
    if (!SocketBase) {
        SocketBase = OpenLibrary("bsdsocket.library", 3);
        if (!SocketBase) {
            SocketBase = OpenLibrary("bsdsocket.library", 0);
        }

        if (!SocketBase) {
            printf("FATAL: Failed to open bsdsocket.library (any version)!\n");
        } else {
            m_OwnedSocketBase = true;
        }
    }

    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);
    if (!IntuitionBase) {
        printf("FATAL: intuition.library v36+ failed to open!\n");
    }

    MUIMasterBase = OpenLibrary("muimaster.library", 19);
    if (!MUIMasterBase) {
        printf("FATAL: muimaster.library v19+ failed to open!\n");
    }

    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
    if (!GfxBase) printf("FATAL: graphics.library failed to open!\n");

    DataTypesBase = OpenLibrary("datatypes.library", 37);
    if (!DataTypesBase) printf("FATAL: datatypes.library failed to open!\n");

    UtilityBase = OpenLibrary("utility.library", 37);
    if (!UtilityBase) printf("FATAL: utility.library failed to open!\n");
}

void App::CleanupMUI() {
    if (SocketBase && m_OwnedSocketBase) {
        CloseLibrary(SocketBase);
        SocketBase = NULL;
    }
    if (UtilityBase) CloseLibrary(UtilityBase);
    if (DataTypesBase) CloseLibrary(DataTypesBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
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
    if (!MUIMasterBase) {
        printf("FATAL: MUIMasterBase is NULL, cannot create App object.\n");
        return;
    }

    m_App = MUIHelpers_NewObject(MUIC_Application,
        MUIA_Application_Title, (IPTR)"Amidon2",
        MUIA_Application_Version, (IPTR)"$VER: 0.1",
        MUIA_Application_Copyright, (IPTR)"(C) 2026 Dimitris Panokostas",
        MUIA_Application_Author, (IPTR)"Dimitris Panokostas",
        MUIA_Application_Description, (IPTR)"Mastodon Client for AmigaOS",
        MUIA_Application_Base, (IPTR)"AMIDON",
        TAG_DONE);

    if (!m_App) {
        fprintf(stderr, "Failed to create MUI Application object\n");
        return;
    }

    m_AsyncHttp = std::make_unique<AsyncHttpService>();
    if (!m_AsyncHttp->Init()) {
        fprintf(stderr, "WARNING: AsyncHttpService init failed\n");
        m_AsyncHttp.reset();
    }

    if (m_AsyncHttp) {
        m_API->SetAsyncHttp(m_AsyncHttp.get());
    }

    // MainWindow tries Guigfx.mcc at header-build time and falls back to
    // Dtpic+Scrollgroup if it's not installed; no probe needed here.
    m_MainWindow = std::make_unique<MainWindow>(*m_API);

    Object* winObj = m_MainWindow->Create();
    if (!winObj) {
        fprintf(stderr, "Failed to create Main Window object\n");
    }

    DoMethod(m_App, OM_ADDMEMBER, (IPTR)winObj);

    m_MainWindow->InitNotifications(m_App);

    DoMethod(m_MainWindow->GetMUIObject(), MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             m_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    std::string instance;
    if (HasSettings()) {
        FILE* f = fopen("settings.json", "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long fsize = ftell(f);
            fseek(f, 0, SEEK_SET);

            char readBuf[2048];
            size_t toRead = (fsize < (long)sizeof(readBuf) - 1) ? (size_t)fsize : sizeof(readBuf) - 1;
            size_t readSize = fread(readBuf, 1, toRead, f);
            fclose(f);
            readBuf[readSize] = 0;

            instance = SettingsParser::ExtractInstance(readBuf);
        }
    }

    if (!instance.empty()) {
        if (CacheManager::CacheExists(instance)) {
            if (CacheManager::LoadCreds(instance, *m_AppRegistration)) {
            }

            std::string token;
            if (CacheManager::LoadToken(instance, token)) {
                m_API->SetCredentials(instance, token);

                m_API->GetAccountInfo([this, instance](bool success, const Account& account) {
                    if (!success) return;

                    if (m_MainWindow) m_MainWindow->SetProfile(account);

                    std::string displayText = account.display_name.empty() ? account.username : account.display_name;
                    std::string avatarPath = CacheManager::GetAvatarPath(instance);
                    struct stat st;
                    bool haveCached = stat(avatarPath.c_str(), &st) == 0;

                    if (haveCached) {
                        m_MainWindow->SetAccountInfo(displayText, instance, avatarPath);
                    } else {
                        // Show name immediately; avatar fills in once downloaded.
                        m_MainWindow->SetAccountInfo(displayText, instance, "");
                        DownloadAvatar(instance, account.avatar,
                            [this, displayText, instance](const std::string& path) {
                                if (!path.empty() && m_MainWindow) {
                                    m_MainWindow->SetAccountInfo(displayText, instance, path);
                                }
                            });
                    }
                });

                ShowMain();
                m_MainWindow->FetchTimeline();
            } else {
                m_API->SetCredentials(instance, "");
                ShowMain();
                ShowLogin();
            }
        } else {
            ShowMain();
            ShowLogin();
        }
    } else {
        ShowWelcome();
    }

    m_Running = true;
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
                            m_API->GetToken(*m_AppRegistration, code, [this](bool success, const std::string& token) {
                                 if (success) {
                                     m_API->SetCredentials(m_AppRegistration->instance_url, token);

                                     std::string instance = m_AppRegistration->instance_url;

                                     CacheManager::SaveCreds(instance,
                                                           m_AppRegistration->client_id,
                                                           m_AppRegistration->client_secret);

                                     CacheManager::SaveToken(instance, token);

                                     json_object* obj = json_object_new_object();
                                     json_object_object_add(obj, "server", json_object_new_string(instance.c_str()));
                                     const char* jsonStr = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY);

                                     FILE* f = fopen("settings.json", "w");
                                     if (f) {
                                         fwrite(jsonStr, 1, strlen(jsonStr), f);
                                         fclose(f);
                                     }
                                     json_object_put(obj);

                                     m_API->GetAccountInfo([this, instance](bool success, const Account& account) {
                                         if (!success) return;

                                         std::string displayText = account.display_name.empty() ? account.username : account.display_name;

                                         // Show name immediately; avatar fills in once downloaded.
                                         m_MainWindow->SetAccountInfo(displayText, instance, "");
                                         DownloadAvatar(instance, account.avatar,
                                             [this, displayText, instance](const std::string& path) {
                                                 if (!path.empty() && m_MainWindow) {
                                                     m_MainWindow->SetAccountInfo(displayText, instance, path);
                                                 }
                                             });
                                     });

                                     if (m_LoginDialog) {
                                           SetAttrs(m_LoginDialog->GetMUIObject(), MUIA_Window_Open, FALSE, TAG_DONE);
                                      }

                                     m_MainWindow->FetchTimeline();
                                     ShowMain();
                                  } else {
                                      fprintf(stderr, "Login failed\n");
                                  }
                              });
                        }
                    }
                }
                break;
            case APPRETURN_REFRESH_TIMELINE:
                if (m_MainWindow) {
                    m_MainWindow->FetchTimeline();
                }
                break;
            case APPRETURN_POST:
                if (m_MainWindow) {
                    m_MainWindow->HandlePost();
                }
                break;
            case APPRETURN_TEXT_CHANGED:
                if (m_MainWindow) {
                    m_MainWindow->UpdateCharacterCounter();
                }
                break;
            case APPRETURN_TIMELINE_SELECT: {
                LONG active = MUIV_List_Active_Off;
                GetAttr(MUIA_List_Active, m_MainWindow->m_ListTimelineInner, (IPTR*)&active);
                if (active != MUIV_List_Active_Off) {
                    m_MainWindow->ShowToot((int)active);
                }
                break;
            }
            case APPRETURN_NAVIGATION: {
                if (!m_MainWindow || !m_MainWindow->m_PageGroup) break;
                LONG active = MUIV_List_Active_Off;
                GetAttr(MUIA_List_Active, m_MainWindow->m_ListNavigationInner, (IPTR*)&active);
                if (active != MUIV_List_Active_Off) {
                    SetAttrs(m_MainWindow->m_PageGroup, MUIA_Group_ActivePage, (IPTR)active, TAG_DONE);
                    // Auto-fetch on switch to a page that has live data.
                    // Indices match the titles[] array in MainWindow::Create:
                    // 0 Publish, 1 Notifications, 2 Explore, 3 Timeline, ...
                    if (active == 1 && m_API && m_API->HasCredentials()) {
                        m_MainWindow->FetchNotifications();
                    } else if (active == 2) {
                        // Explore — trending, no auth required
                        m_MainWindow->FetchExplore();
                    } else if (active == 3) {
                        m_MainWindow->FetchTimeline();
                    } else if (active == 4 && m_API && m_API->HasCredentials()) {
                        // DMs
                        m_MainWindow->FetchDMs();
                    } else if (active == 5 && m_API && m_API->HasCredentials()) {
                        // Favourites
                        m_MainWindow->FetchFavourites();
                    } else if (active == 6 && m_API && m_API->HasCredentials()) {
                        // Bookmarks
                        m_MainWindow->FetchBookmarks();
                    } else if (active == 7 && m_API && m_API->HasCredentials()) {
                        // Lists
                        m_MainWindow->FetchLists();
                    } else if (active == 8 && m_API && m_API->HasCredentials()) {
                        // Requests
                        m_MainWindow->FetchRequests();
                    }
                }
                break;
            }
            case APPRETURN_REFRESH_NOTIFICATIONS:
                if (m_MainWindow) m_MainWindow->FetchNotifications();
                break;
            case APPRETURN_NOTIFICATION_SELECT: {
                if (!m_MainWindow || !m_MainWindow->m_ListNotificationsInner) break;
                LONG active = MUIV_List_Active_Off;
                GetAttr(MUIA_List_Active, m_MainWindow->m_ListNotificationsInner, (IPTR*)&active);
                if (active != MUIV_List_Active_Off) {
                    m_MainWindow->ShowNotification((int)active);
                }
                break;
            }
            case APPRETURN_REFRESH_FAVOURITES:
                if (m_MainWindow) m_MainWindow->FetchFavourites();
                break;
            case APPRETURN_FAVOURITES_SELECT: {
                if (!m_MainWindow || !m_MainWindow->m_ListFavouritesInner) break;
                LONG active = MUIV_List_Active_Off;
                GetAttr(MUIA_List_Active, m_MainWindow->m_ListFavouritesInner, (IPTR*)&active);
                if (active != MUIV_List_Active_Off) {
                    m_MainWindow->ShowFavourite((int)active);
                }
                break;
            }
            case APPRETURN_REFRESH_BOOKMARKS:
                if (m_MainWindow) m_MainWindow->FetchBookmarks();
                break;
            case APPRETURN_BOOKMARKS_SELECT: {
                if (!m_MainWindow || !m_MainWindow->m_ListBookmarksInner) break;
                LONG active = MUIV_List_Active_Off;
                GetAttr(MUIA_List_Active, m_MainWindow->m_ListBookmarksInner, (IPTR*)&active);
                if (active != MUIV_List_Active_Off) {
                    m_MainWindow->ShowBookmark((int)active);
                }
                break;
            }
            case APPRETURN_FAVOURITE_TOOT:
                if (m_MainWindow) m_MainWindow->HandleFavouriteToot();
                break;
            case APPRETURN_BOOST_TOOT:
                if (m_MainWindow) m_MainWindow->HandleBoostToot();
                break;
            case APPRETURN_REFRESH_EXPLORE:
                if (m_MainWindow) m_MainWindow->FetchExplore();
                break;
            case APPRETURN_EXPLORE_SELECT: {
                if (!m_MainWindow || !m_MainWindow->m_ListExploreInner) break;
                LONG active = MUIV_List_Active_Off;
                GetAttr(MUIA_List_Active, m_MainWindow->m_ListExploreInner, (IPTR*)&active);
                if (active != MUIV_List_Active_Off) {
                    m_MainWindow->ShowExploreItem((int)active);
                }
                break;
            }
            case APPRETURN_REFRESH_DMS:
                if (m_MainWindow) m_MainWindow->FetchDMs();
                break;
            case APPRETURN_DMS_SELECT: {
                if (!m_MainWindow || !m_MainWindow->m_ListDMsInner) break;
                LONG active = MUIV_List_Active_Off;
                GetAttr(MUIA_List_Active, m_MainWindow->m_ListDMsInner, (IPTR*)&active);
                if (active != MUIV_List_Active_Off) {
                    m_MainWindow->ShowDM((int)active);
                }
                break;
            }
            case APPRETURN_REFRESH_LISTS:
                if (m_MainWindow) m_MainWindow->FetchLists();
                break;
            case APPRETURN_LISTS_SELECT: {
                if (!m_MainWindow || !m_MainWindow->m_ListListsInner) break;
                LONG active = MUIV_List_Active_Off;
                GetAttr(MUIA_List_Active, m_MainWindow->m_ListListsInner, (IPTR*)&active);
                if (active != MUIV_List_Active_Off) {
                    m_MainWindow->ShowList((int)active);
                }
                break;
            }
            case APPRETURN_REFRESH_REQUESTS:
                if (m_MainWindow) m_MainWindow->FetchRequests();
                break;
            case APPRETURN_REQUESTS_SELECT: {
                if (!m_MainWindow || !m_MainWindow->m_ListRequestsInner) break;
                LONG active = MUIV_List_Active_Off;
                GetAttr(MUIA_List_Active, m_MainWindow->m_ListRequestsInner, (IPTR*)&active);
                if (active != MUIV_List_Active_Off) {
                    m_MainWindow->ShowRequest((int)active);
                }
                break;
            }
            case APPRETURN_SAVE_SETTINGS:
                {
                    std::string instance;
                    if (m_SettingsDialog) {
                         instance = m_SettingsDialog->GetInstanceURL();

                         json_object* obj = json_object_new_object();
                         json_object_object_add(obj, "server", json_object_new_string(instance.c_str()));
                         const char* jsonStr = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY);

                         FILE* f = fopen("settings.json", "w");
                         if (f) {
                             fwrite(jsonStr, 1, strlen(jsonStr), f);
                             fclose(f);
                         } else {
                             fprintf(stderr, "Failed to open settings.json for writing\n");
                         }
                         json_object_put(obj);

                        SetAttrs(m_SettingsDialog->GetMUIObject(), MUIA_Window_Open, FALSE, TAG_DONE);
                    }

                    if (m_WelcomeDialog) {
                        SetAttrs(m_WelcomeDialog->GetMUIObject(), MUIA_Window_Open, FALSE, TAG_DONE);
                    }

                    ShowMain();

                    // Bind the instance to the API and load the public timeline
                    // for the unauthenticated browsing flow. Token stays empty;
                    // the user can still log in afterwards from the menu.
                    if (!instance.empty()) {
                        m_API->SetCredentials(instance, "");
                        m_MainWindow->FetchTimeline();
                    }
                }
                break;
        }

        if (m_Running) {
            amidon_fd_set readFds, writeFds;
            AMIDON_FD_ZERO(&readFds);
            AMIDON_FD_ZERO(&writeFds);
            int nfds = m_AsyncHttp ? m_AsyncHttp->GetSelectFdSets(&readFds, &writeFds) : 0;

            if (nfds > 0) {
                struct timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 100000;
                int sel = WaitSelect(nfds,
                    (APTR)&readFds, (APTR)&writeFds, NULL, &tv, &signals);
                if (sel > 0) {
                    if (m_AsyncHttp) {
                        m_AsyncHttp->Progress();
                    }
                }
                if (signals & SIGBREAKF_CTRL_C) break;
            } else if (signals) {
                Wait(signals);
                if (signals & SIGBREAKF_CTRL_C) break;
            }
        }
    }
}

void App::ShowMain() {
    if (m_MainWindow) {
        Object* win = m_MainWindow->GetMUIObject();
        if (win) {
            SetAttrs(win, MUIA_Window_Open, TRUE, TAG_DONE);
        }
    }
}

void App::ShowWelcome() {
    if (!m_WelcomeDialog) {
        m_WelcomeDialog = std::make_unique<WelcomeDialog>();
        DoMethod(m_App, OM_ADDMEMBER, (IPTR)m_WelcomeDialog->Create());
        m_WelcomeDialog->InitNotifications(m_App);
    }
    SetAttrs(m_WelcomeDialog->GetMUIObject(), MUIA_Window_Open, TRUE, TAG_DONE);
}

void App::ShowSettings() {
    if (!m_SettingsDialog) {
        m_SettingsDialog = std::make_unique<SettingsDialog>();
        DoMethod(m_App, OM_ADDMEMBER, (IPTR)m_SettingsDialog->Create());
        m_SettingsDialog->InitNotifications(m_App);
    }
    SetAttrs(m_SettingsDialog->GetMUIObject(), MUIA_Window_Open, TRUE, TAG_DONE);
}

void App::DownloadAvatar(const std::string& instance, const std::string& url,
                          std::function<void(const std::string& path)> done) {
    if (!m_AsyncHttp || url.empty()) {
        if (done) done("");
        return;
    }

    fprintf(stderr, "DEBUG: DownloadAvatar url=%s\n", url.c_str());
    m_AsyncHttp->Get(url, "", [instance, done](const std::string& body, long code) {
        if (code != 200 || body.empty()) {
            fprintf(stderr, "DEBUG: DownloadAvatar failed code=%ld len=%zu\n", code, body.length());
            if (done) done("");
            return;
        }
        if (!CacheManager::WriteAvatarFile(instance, body)) {
            if (done) done("");
            return;
        }
        if (done) done(CacheManager::GetAvatarPath(instance));
    });
}

void App::ShowLogin() {
    if (!m_LoginDialog) {
        m_LoginDialog = std::make_unique<LoginDialog>();
        DoMethod(m_App, OM_ADDMEMBER, (IPTR)m_LoginDialog->Create());
        m_LoginDialog->InitNotifications(m_App);
    }
    SetAttrs(m_LoginDialog->GetMUIObject(), MUIA_Window_Open, TRUE, TAG_DONE);

    std::string instance;
    if (HasSettings()) {
        FILE* f = fopen("settings.json", "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long fsize = ftell(f);
            fseek(f, 0, SEEK_SET);

            char readBuf[2048];
            size_t toRead = (fsize < (long)sizeof(readBuf) - 1) ? (size_t)fsize : sizeof(readBuf) - 1;
            size_t readSize = fread(readBuf, 1, toRead, f);
            fclose(f);
            readBuf[readSize] = 0;

            instance = SettingsParser::ExtractInstance(readBuf);
        }
    }

    if (instance.empty()) {
        return;
    }

    if (m_AppRegistration->client_id.empty()) {
        m_LoginDialog->SetAuthURL("Registering...");
        m_API->RegisterApp(instance, [this](bool success, const AppRegistration& reg) {
            if (success) {
                *m_AppRegistration = reg;
                std::string authUrl = UrlBuilder::OAuthAuthorize(reg.instance_url, reg.client_id);
                if (m_LoginDialog) {
                    m_LoginDialog->SetAuthURL(authUrl.c_str());
                }
            } else {
                if (m_LoginDialog) {
                    m_LoginDialog->SetAuthURL("Registration Failed!");
                }
            }
        });
    } else {
        std::string authUrl = UrlBuilder::OAuthAuthorize(m_AppRegistration->instance_url, m_AppRegistration->client_id);
        m_LoginDialog->SetAuthURL(authUrl.c_str());
    }
}
