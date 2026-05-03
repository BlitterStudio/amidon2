#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include "../MastodonAPI.h"
#include <proto/muimaster.h>
#include <libraries/mui.h>

#include <string>
#include <vector>
#include <list>

extern "C" {
#include <clib/alib_protos.h>
#include "HTMLview_mcc.h"
#include "htmlview_nethook.h"
}

class MastodonAPI;

class MainWindow {
public:
    MainWindow(MastodonAPI& api);
    Object* Create();
    void FetchTimeline();
    void FetchNotifications();
    void FetchFavourites();
    void FetchBookmarks();
    void FetchExplore();
    void FetchDMs();
    void FetchLists();
    void FetchRequests();
    void HandlePost();
    void SetAccountInfo(const std::string& username, const std::string& instance, const std::string& avatarPath);
    void UpdateCharacterCounter();
    void ShowToot(int index);
    void ShowNotification(int index);
    void ShowFavourite(int index);
    void ShowBookmark(int index);
    void ShowExploreItem(int index);
    void ShowDM(int index);
    void ShowList(int index);
    void ShowRequest(int index);

    // Toot interactions on the currently-selected Timeline entry.
    void HandleFavouriteToot();
    void HandleBoostToot();
    void SetProfile(const Account& account);

    // Forwards a click on an <a> inside any HTMLview pane. The URL and the
    // anchor's class= attribute (when present — e.g. Mastodon's "mention"
    // or "hashtag") are looked up from the HTMLview that fired the event.
    void HandleLinkClicked();
    void InitNotifications(Object* app);
    Object* GetMUIObject() { return m_Window; }

    Object* m_Window;
    Object* m_ListTimeline;
    Object* m_ListTimelineInner;
    Object* m_ListNavigation;
    Object* m_ListNavigationInner;
    Object* m_PageGroup;

    Object* m_BtnRefreshTimeline;
    Object* m_BtnFavouriteToot;
    Object* m_BtnBoostToot;
    Object* m_BtnRefreshNotifications;
    Object* m_ListNotifications;
    Object* m_ListNotificationsInner;

    Object* m_BtnRefreshFavourites;
    Object* m_ListFavourites;
    Object* m_ListFavouritesInner;
    Object* m_HtmlviewFavourites;

    Object* m_BtnRefreshBookmarks;
    Object* m_ListBookmarks;
    Object* m_ListBookmarksInner;
    Object* m_HtmlviewBookmarks;

    Object* m_BtnRefreshExplore;
    Object* m_ListExplore;
    Object* m_ListExploreInner;
    Object* m_HtmlviewExplore;

    Object* m_BtnRefreshDMs;
    Object* m_ListDMs;
    Object* m_ListDMsInner;
    Object* m_HtmlviewDMs;

    Object* m_BtnRefreshLists;
    Object* m_ListLists;
    Object* m_ListListsInner;
    Object* m_HtmlviewLists;

    Object* m_BtnRefreshRequests;
    Object* m_ListRequests;
    Object* m_ListRequestsInner;
    Object* m_HtmlviewRequests;

    Object* m_LabelProfileDisplay;
    Object* m_LabelProfileAcct;
    Object* m_LabelProfileStats;
    Object* m_HtmlviewProfileBio;

    Object* m_LabelUsername;
    Object* m_LabelInstance;
    Object* m_ImageAvatar;
    Object* m_HeaderGroup;

    Object* m_TextEditor;
    Object* m_LabelCharCount;
    Object* m_CycleVisibility;
    Object* m_CheckCW;
    Object* m_StringCW;
    Object* m_BtnPost;

    Object* m_Htmlview;
    Object* m_HtmlviewNotifications;
    struct Hook m_NetHook;
    struct Hook m_LinkHook;

    Object* m_ItemQuit;
    Object* m_ItemSettings;
    Object* m_ItemMUI;

    // Data Management
    std::list<std::string> m_TimelineStrings;
    std::vector<Status> m_TimelineData;

    std::list<std::string> m_NotificationStrings;
    std::vector<Notification> m_NotificationData;

    std::list<std::string> m_FavouritesStrings;
    std::vector<Status> m_FavouritesData;

    std::list<std::string> m_BookmarksStrings;
    std::vector<Status> m_BookmarksData;

    std::list<std::string> m_ExploreStrings;
    std::vector<Status> m_ExploreData;

    std::list<std::string> m_DMsStrings;
    std::vector<Status> m_DMsData;

    std::list<std::string> m_ListsStrings;
    std::vector<MastodonList> m_ListsData;

    std::list<std::string> m_RequestsStrings;
    std::vector<Account> m_RequestsData;

    struct Hook m_TextEditorHook;

private:
    MastodonAPI& m_API;
    Object* m_App;
    bool m_UseGuigfx;

    Object* CreateHeader();
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
    Object* CreateMenu();

    static unsigned long LinkClickHookFn(struct Hook* hook, Object* htmlview, void* msg);
};

#endif // UI_MAINWINDOW_H
