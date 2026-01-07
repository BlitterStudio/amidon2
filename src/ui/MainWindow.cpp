/*
 * Amidon - a Mastodon client for AmigaOS
 * Copyright (C) 2024 Dimitris Panokostas
 */

#include "MainWindow.h"
#include "MUIHelpers.h"
#include "../Constants.h"

#include <proto/muimaster.h>
#include <libraries/mui.h>

MainWindow::MainWindow(MastodonAPI& api) : m_Window(nullptr), m_API(api) {
    printf("DEBUG: MainWindow constructor starting...\n");
    Create();
    printf("DEBUG: MainWindow constructor completed.\n");
}

Object* MainWindow::CreateHeader() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, MUIA_FixWidth, 32, MUIA_FixHeight, 32, TAG_DONE), // Placeholder for Avatar
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, FALSE,
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"Username", TAG_DONE),
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"Instance", TAG_DONE),
            TAG_DONE
        ),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, TAG_DONE), // Spacer
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, MUIA_FixWidth, 32, MUIA_FixHeight, 32, TAG_DONE), // Placeholder for Logo
        TAG_DONE
    );
}

Object* MainWindow::CreatePublishPage() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, 
            MUIA_Text_Contents, (IPTR)"Compose new status", 
            TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_String, 
            MUIA_Frame, MUIV_Frame_String,
            MUIA_ControlChar, 's',
            TAG_DONE), // Placeholder for multi-line text entry
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, 
                MUIA_Text_Contents, (IPTR)"Button Bar (TODO)", 
                TAG_DONE),
            TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, TAG_DONE), // Vertical Spacer
        TAG_DONE
    );
}

Object* MainWindow::CreateTimelinePage() {
    m_ListTimeline = MUIHelpers_NewObject(MUIC_List,
        MUIA_Frame, MUIV_Frame_InputList,
        TAG_DONE);

    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Listview,
            MUIA_Listview_List, (IPTR)m_ListTimeline,
            TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_Child, (IPTR)(m_BtnRefreshTimeline = MakeImageButton("PROGDIR:assets/Refresh.png")),
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"Load More", TAG_DONE),
            TAG_DONE),
        TAG_DONE
    );
}

void MainWindow::FetchTimeline() {
    // printf("DEBUG: MainWindow::FetchTimeline called.\n");
    // Show busy state ideally... 
    
    auto callback = [this](std::vector<Status> timeline) {
        printf("DEBUG: Timeline callback received. Items: %lu\n", timeline.size());
        
        // Clear list
        DoMethod(m_ListTimeline, MUIM_List_Clear);
        
        // Clear underlying string storage
        m_TimelineStrings.clear();
        
        for (const auto& status : timeline) {
             std::string line = status.author_username + ": " + status.content;
             // printf("DEBUG: Adding timeline item: %s\n", line.c_str()); 
             
             // Store in vector to keep alive
             m_TimelineStrings.push_back(line);
             
             // Use pointer from vector
             DoMethod(m_ListTimeline, MUIM_List_InsertSingle, (IPTR)m_TimelineStrings.back().c_str(), MUIV_List_Insert_Bottom);
        }
    };

    if (m_API.HasCredentials()) {
        // printf("DEBUG: Fetching Home Timeline...\n");
        m_API.GetHomeTimeline(callback);
    } else {
        // printf("DEBUG: Fetching Public Timeline (Not Authenticated)...\n");
        m_API.GetPublicTimeline(callback);
    }
}

Object* MainWindow::CreateNotificationsPage() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Listview,
            MUIA_Listview_List, (IPTR)MUIHelpers_NewObject(MUIC_List,
                MUIA_Frame, MUIV_Frame_InputList,
                TAG_DONE),
            TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"Refresh", TAG_DONE),
            TAG_DONE),
        TAG_DONE
    );
}

Object* MainWindow::CreateExplorePage() {
    static const char* titles[] = { "Posts", "Hashtags", "News", "For You", NULL };

    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"Refresh", TAG_DONE),
            TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Register,
            MUIA_Register_Titles, (IPTR)titles,
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Listview,
                MUIA_Listview_List, (IPTR)MUIHelpers_NewObject(MUIC_List, MUIA_Frame, MUIV_Frame_InputList, TAG_DONE),
                TAG_DONE),
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Listview,
                MUIA_Listview_List, (IPTR)MUIHelpers_NewObject(MUIC_List, MUIA_Frame, MUIV_Frame_InputList, TAG_DONE),
                TAG_DONE),
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Listview,
                MUIA_Listview_List, (IPTR)MUIHelpers_NewObject(MUIC_List, MUIA_Frame, MUIV_Frame_InputList, TAG_DONE),
                TAG_DONE),
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Listview,
                MUIA_Listview_List, (IPTR)MUIHelpers_NewObject(MUIC_List, MUIA_Frame, MUIV_Frame_InputList, TAG_DONE),
                TAG_DONE),
            TAG_DONE),
        TAG_DONE
    );
}

Object* MainWindow::CreateDMSPage() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"Refresh", TAG_DONE),
            TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Listview,
            MUIA_Listview_List, (IPTR)MUIHelpers_NewObject(MUIC_List, MUIA_Frame, MUIV_Frame_InputList, TAG_DONE),
            TAG_DONE),
        TAG_DONE
    );
}

Object* MainWindow::CreateFavouritesPage() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_Child, (IPTR)MakeImageButton("PROGDIR:assets/Refresh.png"),
            TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Listview,
            MUIA_Listview_List, (IPTR)MUIHelpers_NewObject(MUIC_List, MUIA_Frame, MUIV_Frame_InputList, TAG_DONE),
            TAG_DONE),
        TAG_DONE
    );
}

Object* MainWindow::CreateBookmarksPage() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"Refresh", TAG_DONE),
            TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Listview,
            MUIA_Listview_List, (IPTR)MUIHelpers_NewObject(MUIC_List, MUIA_Frame, MUIV_Frame_InputList, TAG_DONE),
            TAG_DONE),
        TAG_DONE
    );
}

Object* MainWindow::CreateListsPage() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"Refresh", TAG_DONE),
            TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Listview,
            MUIA_Listview_List, (IPTR)MUIHelpers_NewObject(MUIC_List, MUIA_Frame, MUIV_Frame_InputList, TAG_DONE),
            TAG_DONE),
        TAG_DONE
    );
}

Object* MainWindow::CreateRequestsPage() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"Refresh", TAG_DONE),
            TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Listview,
            MUIA_Listview_List, (IPTR)MUIHelpers_NewObject(MUIC_List, MUIA_Frame, MUIV_Frame_InputList, TAG_DONE),
            TAG_DONE),
        TAG_DONE
    );
}

Object* MainWindow::CreateProfilePage() {
    static const char* titles[] = { "Posts", "Posts and Replies", NULL };

    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"User Profile Info (Avatar, Stats)", TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"Refresh", TAG_DONE),
            TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Register,
            MUIA_Register_Titles, (IPTR)titles,
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Listview,
                MUIA_Listview_List, (IPTR)MUIHelpers_NewObject(MUIC_List, MUIA_Frame, MUIV_Frame_InputList, TAG_DONE),
                TAG_DONE),
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Listview,
                MUIA_Listview_List, (IPTR)MUIHelpers_NewObject(MUIC_List, MUIA_Frame, MUIV_Frame_InputList, TAG_DONE),
                TAG_DONE),
            TAG_DONE),
        TAG_DONE
    );
}

Object* MainWindow::CreateMenu() {
    Object  *itemAbout;
    Object *menu;

    menu = MUIHelpers_NewObject(MUIC_Menustrip,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Menu,
            MUIA_Menu_Title, (IPTR)"Project",
            MUIA_Group_Child, (IPTR)(itemAbout = MUIHelpers_NewObject(MUIC_Menuitem,
                MUIA_Menuitem_Title, (IPTR)"About...",
                TAG_DONE)),
            MUIA_Group_Child, (IPTR)(m_ItemMUI = MUIHelpers_NewObject(MUIC_Menuitem,
                MUIA_Menuitem_Title, (IPTR)"About MUI...",
                TAG_DONE)),
            MUIA_Group_Child, (IPTR)(m_ItemQuit = MUIHelpers_NewObject(MUIC_Menuitem,
                MUIA_Menuitem_Title, (IPTR)"Quit",
                MUIA_Menuitem_Shortcut, (IPTR)"Q",
                TAG_DONE)),
            TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Menu,
            MUIA_Menu_Title, (IPTR)"Settings",
            MUIA_Group_Child, (IPTR)(m_ItemSettings = MUIHelpers_NewObject(MUIC_Menuitem,
                MUIA_Menuitem_Title, (IPTR)"Settings...",
                MUIA_Menuitem_Shortcut, (IPTR)"S",
                TAG_DONE)),
            TAG_DONE),
        TAG_DONE
    );
    
    return menu;
}

void MainWindow::InitNotifications(Object* app) {
    if (!app) return;
    
    // Quit
    DoMethod(m_ItemQuit, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, 
        app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
        
    // Settings
    DoMethod(m_ItemSettings, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
        app, 2, MUIM_Application_ReturnID, APPRETURN_SETTINGS);
        
    // MUI Settings
    DoMethod(m_ItemMUI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
        app, 2, MUIM_Application_OpenConfigWindow, 0);
        
    // Timeline Refresh
    if (m_BtnRefreshTimeline) {
        DoMethod(m_BtnRefreshTimeline, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, APPRETURN_REFRESH_TIMELINE);
    }
}

Object* MainWindow::Create() {
    // printf("DEBUG: MainWindow::Create starting...\n");
    static const char* titles[] = { 
        "Publish", 
        "Notifications", 
        "Explore",
        "Timeline", 
        "DMs",
        "Favourites",
        "Bookmarks",
        "Lists",
        "Requests",
        "Profile",
        NULL 
    };

    // Create Menu
    // printf("DEBUG: Creating Menu...\n");
    Object* menu = CreateMenu();
    if (!menu) printf("FATAL: Failed to create Menu!\n");
    // else printf("DEBUG: Menu created: %p\n", menu);

    // Create Root Group
    // printf("DEBUG: Creating Root Group...\n");
    
    // Create Navigation List
    // printf("DEBUG: Creating Navigation List...\n");
    m_ListNavigation = MUIHelpers_NewObject(MUIC_List,
                        MUIA_Frame, MUIV_Frame_InputList,
                        MUIA_Background, MUII_ListBack,
                        TAG_DONE);
    if (!m_ListNavigation) printf("FATAL: Failed to create ListNavigation!\n");
    
    // Create Page Group
    // printf("DEBUG: Creating Page Group...\n");
    m_PageGroup = MUIHelpers_NewObject(MUIC_Group,
                    MUIA_Group_PageMode, TRUE,
                    MUIA_Weight, 75,
                    MUIA_Group_Child, (IPTR)CreatePublishPage(),
                    MUIA_Group_Child, (IPTR)CreateNotificationsPage(),
                    MUIA_Group_Child, (IPTR)CreateExplorePage(),
                    MUIA_Group_Child, (IPTR)CreateTimelinePage(),
                    MUIA_Group_Child, (IPTR)CreateDMSPage(),
                    MUIA_Group_Child, (IPTR)CreateFavouritesPage(),
                    MUIA_Group_Child, (IPTR)CreateBookmarksPage(),
                    MUIA_Group_Child, (IPTR)CreateListsPage(),
                    MUIA_Group_Child, (IPTR)CreateRequestsPage(),
                    MUIA_Group_Child, (IPTR)CreateProfilePage(),
                    TAG_DONE);
    if (!m_PageGroup) printf("FATAL: Failed to create PageGroup!\n");

    Object* rootGroup = MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, FALSE,
            MUIA_Group_Child, (IPTR)CreateHeader(),
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
                MUIA_Group_Horiz, TRUE,
                MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Listview,
                    MUIA_Listview_List, (IPTR)(m_ListNavigation),
                    MUIA_Weight, 25,
                    TAG_DONE),
                MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Balance, TAG_DONE),
                MUIA_Group_Child, (IPTR)m_PageGroup,
                TAG_DONE
            ),
            TAG_DONE
        );
    
    if (!rootGroup) printf("FATAL: Failed to create Root Group!\n");
    // else printf("DEBUG: Root Group created: %p\n", rootGroup);

    // printf("DEBUG: Creating Main Window Object...\n");
    m_Window = MUIHelpers_NewObject(MUIC_Window,
        MUIA_Window_Title, (IPTR)"Amidon",
        MUIA_Window_ID, 0x4D41494E, // 'MAIN'
        MUIA_Window_ScreenTitle, (IPTR)"Amidon - Mastodon Client",
        MUIA_Window_SizeGadget, TRUE,
        MUIA_Window_Menustrip, (IPTR)menu,
        MUIA_Window_RootObject, (IPTR)rootGroup,
        TAG_DONE
    );

    // Populate Navigation List
    for (int i = 0; titles[i]; ++i) {
        DoMethod(m_ListNavigation, MUIM_List_InsertSingle, (IPTR)titles[i], MUIV_List_Insert_Bottom);
    }
    
    // Setup Navigation Notification
    DoMethod(m_ListNavigation, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
        m_PageGroup, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue);

    // Set initial selection
    SetAttrs(m_ListNavigation, MUIA_List_Active, 0, TAG_DONE);

    
    // Initial fetch
    FetchTimeline();
    
    return m_Window;
}
