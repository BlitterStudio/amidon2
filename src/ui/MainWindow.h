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
    MainWindow(MastodonAPI& api, bool useGuigfx = false);
    Object* Create();
    void FetchTimeline();
    void HandlePost();
    void SetAccountInfo(const std::string& username, const std::string& instance, const std::string& avatarPath);
    void UpdateCharacterCounter();
    void ShowToot(int index);
    void InitNotifications(Object* app);
    Object* GetMUIObject() { return m_Window; }

    Object* m_Window;
    Object* m_ListTimeline;
    Object* m_ListTimelineInner;
    Object* m_ListNavigation;
    Object* m_PageGroup;

    Object* m_BtnRefreshTimeline;

    Object* m_LabelUsername;
    Object* m_LabelInstance;
    Object* m_ImageAvatar;

    Object* m_TextEditor;
    Object* m_LabelCharCount;
    Object* m_CycleVisibility;
    Object* m_CheckCW;
    Object* m_StringCW;
    Object* m_BtnPost;

    Object* m_Htmlview;
    struct Hook m_NetHook;

    Object* m_ItemQuit;
    Object* m_ItemSettings;
    Object* m_ItemMUI;

    // Data Management
    std::list<std::string> m_TimelineStrings;
    std::vector<Status> m_TimelineData;

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
};

#endif // UI_MAINWINDOW_H
