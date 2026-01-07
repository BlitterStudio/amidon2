/*
 * Amidon - a Mastodon client for AmigaOS
 * Copyright (C) 2024 Dimitris Panokostas
 */

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <libraries/mui.h>
#include "../MastodonAPI.h"
#include <vector>
#include <string>

#ifndef IPTR
typedef unsigned long IPTR;
#endif

class MainWindow {
public:
    MainWindow();
    Object* Create();
    void FetchTimeline();
    Object* GetMUIObject() { return m_Window; }


    // UI Elements
    Object* m_Window;
    Object* m_ListTimeline;
    Object* m_ListNavigation;
    Object* m_PageGroup;
    
    // Buttons
    Object* m_BtnRefreshTimeline;
    
    // Data Management
    std::vector<std::string> m_TimelineStrings;
    
    // Menu Items
    Object* m_ItemQuit;
    Object* m_ItemSettings;
    Object* m_ItemMUI;
    
    // Pages
    struct {
        Object* Publish;
        Object* Notifications;
        Object* Explore;
        Object* Timeline;
        Object* DMS;
        Object* Favourites;
        Object* Bookmarks;
        Object* Lists;
        Object* Requests;
        Object* Profile;
    } m_Pages;

    // Page Groups
    Object* CreatePublishPage();
    Object* CreateNotificationsPage();
    Object* CreateExplorePage();
    Object* CreateTimelinePage();
    Object* CreateDMSPage();
    Object* CreateFavouritesPage();
    Object* CreateBookmarksPage();
    Object* CreateListsPage();
    Object* CreateRequestsPage();
    Object* CreateProfilePage();
    
    // Helpers
    Object* CreateMenu();
    void InitNotifications(Object* app);
    Object* CreateHeader();

    MastodonAPI& m_API;
    
    MainWindow(MastodonAPI& api);
};

#endif // UI_MAINWINDOW_H
