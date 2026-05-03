/*
 * Amidon2 - a Mastodon client for AmigaOS
 * Copyright (C) 2026 Dimitris Panokostas
 */

#include "MainWindow.h"
#include "MUIHelpers.h"
#include "../logic/TootFormatter.h"
#include <mui/TextEditor_mcc.h>
#include <mui/Guigfx_mcc.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include "../Constants.h"
#include "HTMLview_mcc.h"
#include "htmlview_nethook.h"

#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <mui/TextEditor_mcc.h>

#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif

#ifndef MUIC_Dtpic
#define MUIC_Dtpic "Dtpic.mui"
#endif

MainWindow::MainWindow(MastodonAPI& api, bool useGuigfx) 
    : m_Window(nullptr),
      m_ListTimeline(nullptr), m_ListTimelineInner(nullptr), m_ListNavigation(nullptr), m_PageGroup(nullptr),
      m_BtnRefreshTimeline(nullptr), m_LabelUsername(nullptr), m_LabelInstance(nullptr),
      m_ImageAvatar(nullptr), m_TextEditor(nullptr), m_LabelCharCount(nullptr),
      m_CycleVisibility(nullptr), m_CheckCW(nullptr), m_StringCW(nullptr), m_BtnPost(nullptr),
      m_Htmlview(nullptr), m_NetHook({}),
      m_ItemQuit(nullptr), m_ItemSettings(nullptr), m_ItemMUI(nullptr),
      m_API(api), m_App(nullptr), m_UseGuigfx(useGuigfx)
{
    HTMLviewNet_InitHook(&m_NetHook);
}

Object* MainWindow::CreateHeader() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Child, (IPTR)(m_UseGuigfx ? 
            (m_ImageAvatar = (Object *)NewObject(NULL, (CONST_STRPTR)MUIC_Guigfx,
                MUIA_Guigfx_FileName, (IPTR)NULL,
                MUIA_Guigfx_ScaleMode, (IPTR)(NISMF_SCALEFREE | NISMF_KEEPASPECT_PICTURE),
                MUIA_Guigfx_Quality, (IPTR)MUIV_Guigfx_Quality_Low,
                MUIA_FixWidth, 48,
                MUIA_FixHeight, 48,
                TAG_DONE)) :
            MUIHelpers_NewObject(MUIC_Scrollgroup,
                MUIA_Scrollgroup_Contents, (IPTR)MUIHelpers_NewObject(MUIC_Virtgroup,
                    MUIA_Group_Child, (IPTR)(m_ImageAvatar = MUIHelpers_NewObject(MUIC_Dtpic,
                        MUIA_Dtpic_Name, (IPTR)"",
                        TAG_DONE)),
                    TAG_DONE),
                MUIA_Scrollgroup_NoHorizBar, TRUE,
                MUIA_Scrollgroup_NoVertBar, TRUE,
                MUIA_FixWidth, 48, 
                MUIA_FixHeight, 48,
                TAG_DONE)),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, FALSE,
            MUIA_Group_Child, (IPTR)(m_LabelUsername = MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"Username", TAG_DONE)),
            MUIA_Group_Child, (IPTR)(m_LabelInstance = MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"Instance", TAG_DONE)),
            TAG_DONE
        ),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, TAG_DONE), // Spacer
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, MUIA_FixWidth, 32, MUIA_FixHeight, 32, TAG_DONE), // Placeholder for Logo
        TAG_DONE
    );
}

Object* MainWindow::CreatePublishPage() {
    static const char* visibilityLabels[] = { "Public", "Unlisted", "Private", "Direct", NULL };

    // Try creating TextEditor.mcc
    m_TextEditor = MUIHelpers_NewObject("TextEditor.mcc",
        MUIA_CycleChain, 1,
        MUIA_Weight, 100,
        TAG_DONE);

    Object* editorUI = m_TextEditor; // Default to the object itself

    if (m_TextEditor) {
        printf("DEBUG: TextEditor.mcc created successfully.\n");
    } else {
        printf("DEBUG: TextEditor.mcc failed. Falling back to String gadget.\n");
        m_TextEditor = MUIHelpers_NewObject(MUIC_String,
            MUIA_Frame, MUIV_Frame_String,
            MUIA_Weight, 100,
            MUIA_String_Contents, (IPTR)"TextEditor.mcc not found",
            TAG_DONE);
            
        // Wrap fallback String in a group to allow vertical expansion
        if (m_TextEditor) {
            editorUI = MUIHelpers_NewObject(MUIC_Group,
                MUIA_Group_Child, (IPTR)m_TextEditor,
                MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, TAG_DONE),
                TAG_DONE);
        }
    }

    Object* page = MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, FALSE,
            MUIA_Frame, MUIV_Frame_Group,
            MUIA_Frame, MUIV_Frame_Group,
            MUIA_FrameTitle, (IPTR)"Compose Post",
            // Text Editor
            MUIA_Group_Child, (IPTR)editorUI,
            // Controls row
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
                MUIA_Group_Horiz, TRUE,
                MUIA_Group_Child, (IPTR)(m_LabelCharCount = MUIHelpers_NewObject(MUIC_Text,
                    MUIA_Text_Contents, (IPTR)"500 characters remaining",
                    TAG_DONE)),
                MUIA_Group_Child, (IPTR)(m_CycleVisibility = MUIHelpers_NewObject(MUIC_Cycle,
                    MUIA_Cycle_Entries, (IPTR)visibilityLabels,
                    TAG_DONE)),
                TAG_DONE),
            TAG_DONE),
        // Options Group
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, FALSE,
            MUIA_Frame, MUIV_Frame_Group,
            MUIA_FrameTitle, (IPTR)"Options",
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
                MUIA_Group_Horiz, TRUE,
                MUIA_Group_Child, (IPTR)(m_CheckCW = MUIHelpers_NewObject(MUIC_Image,
                    MUIA_Frame, MUIV_Frame_ImageButton,
                    MUIA_InputMode, MUIV_InputMode_Toggle,
                    MUIA_Image_Spec, MUII_CheckMark,
                    MUIA_Image_FreeVert, TRUE,
                    MUIA_Selected, FALSE,
                    MUIA_Background, MUII_ButtonBack,
                    MUIA_ShowSelState, FALSE,
                    TAG_DONE)),
                MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text,
                    MUIA_Text_Contents, (IPTR)"Content Warning:",
                    TAG_DONE)),
            MUIA_Group_Child, (IPTR)(m_StringCW = MUIHelpers_NewObject(MUIC_String,
                MUIA_Frame, MUIV_Frame_String,
                MUIA_Disabled, TRUE,
                TAG_DONE)),
            TAG_DONE),
        // Actions
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, TAG_DONE),
            MUIA_Group_Child, (IPTR)(m_BtnPost = (Object *)MUIHelpers_NewObject(MUIC_Text,
                MUIA_Frame, MUIV_Frame_Button,
                MUIA_Background, MUII_ButtonBack,
                MUIA_InputMode, MUIV_InputMode_RelVerify,
                MUIA_Text_Contents, (IPTR)"\33cPost Status",
                TAG_DONE)),
            TAG_DONE),
        TAG_DONE);

    if (!page) printf("FATAL: CreatePublishPage failed! possibly due to CheckMark or TextEditor\n");
    return page;
}

Object* MainWindow::CreateTimelinePage() {

    m_ListTimelineInner = MUIHelpers_NewObject(MUIC_List, TAG_DONE);
    if (!m_ListTimelineInner) {
        printf("FATAL: Failed to create m_ListTimelineInner!\n");
        return nullptr;
    }

    Object* htmlScrollgroup = MUIHelpers_NewObject(MUIC_Scrollgroup,
        MUIA_Scrollgroup_FreeVert, TRUE,
        MUIA_Scrollgroup_FreeHoriz, TRUE,
        MUIA_Scrollgroup_Contents, m_Htmlview = MUIHelpers_NewObject(MUIC_HTMLview,
            MUIA_Background, MUII_TextBack,
            MUIA_HTMLview_ImageLoadHook, (IPTR)&m_NetHook,
            MUIA_HTMLview_LoadHook, (IPTR)&m_NetHook,
            MUIA_HTMLview_Contents, (IPTR)"",
            TAG_DONE),
        TAG_DONE);


    Object* bottomChild;
    if (htmlScrollgroup && m_Htmlview) {
        bottomChild = htmlScrollgroup;
    } else {
        bottomChild = MUIHelpers_NewObject(MUIC_Rectangle,
            MUIA_Frame, MUIV_Frame_None,
            TAG_DONE);
    }

    Object* page = MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)(m_BtnRefreshTimeline = (Object *)MUIHelpers_NewObject(MUIC_Text,
            MUIA_Frame, MUIV_Frame_Button,
            MUIA_Background, MUII_ButtonBack,
            MUIA_InputMode, MUIV_InputMode_RelVerify,
            MUIA_Text_Contents, (IPTR)"\33cRefresh Timeline",
            TAG_DONE)),
        MUIA_Group_Child, (IPTR)(m_ListTimeline = MUIHelpers_NewObject(MUIC_Listview,
            MUIA_Weight, 50,
            MUIA_Listview_List, (IPTR)m_ListTimelineInner,
            TAG_DONE)),
        MUIA_Group_Child, (IPTR)bottomChild,
        TAG_DONE);

    if (!page) printf("FATAL: CreateTimelinePage failed!\n");
    return page;
}

void MainWindow::FetchTimeline() {
    // printf("DEBUG: MainWindow::FetchTimeline called.\n");
    // Show busy state ideally... 
    
    auto callback = [this](std::vector<Status> timeline) {
        Object *listObj = m_ListTimelineInner;
        
        if (!listObj) {
            return;
        }

        // Clear list
        DoMethod(listObj, MUIM_List_Clear);
        
        m_TimelineData = timeline;
        
        // Clear underlying string storage
        m_TimelineStrings.clear();
        
        for (const auto& status : timeline) {
             std::string line = status.author_username + ": " + status.content;
             
             // Store in vector to keep alive
             m_TimelineStrings.push_back(line);
             
             // Use pointer from vector
             DoMethod(listObj, MUIM_List_InsertSingle, (IPTR)m_TimelineStrings.back().c_str(), MUIV_List_Insert_Bottom);
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

void MainWindow::HandlePost() {
    if (!m_API.HasCredentials()) {
        printf("ERROR: Cannot post - not authenticated\n");
        // TODO: Show MUI requester to user
        return;
    }
    
    // Get post content from TextEditor
    char* contentPtr = nullptr;
    // Try TextEditor method first
    DoMethod(m_TextEditor, MUIM_TextEditor_ExportText, &contentPtr);
    
    // If pointer is still null, try String gadget content (fallback)
    if (!contentPtr) {
        GetAttr(MUIA_String_Contents, m_TextEditor, (IPTR*)&contentPtr);
    }

    std::string content = contentPtr ? contentPtr : "";
    
    // Validate content
    if (content.empty()) {
        printf("ERROR: Cannot post empty status\n");
        // TODO: Show MUI requester to user
        return;
    }
    
    if (content.length() > 500) {
        printf("ERROR: Post exceeds 500 character limit\n");
        // TODO: Show MUI requester to user
        return;
    }
    
    // Get visibility setting
    LONG visibilityIndex = 0;
    GetAttr(MUIA_Cycle_Active, m_CycleVisibility, (IPTR*)&visibilityIndex);
    
    static const char* visibilityValues[] = { "public", "unlisted", "private", "direct" };
    std::string visibility = visibilityValues[visibilityIndex];
    
    // Get content warning if enabled
    std::string spoilerText;
    LONG cwEnabled = 0;
    GetAttr(MUIA_Selected, m_CheckCW, (IPTR*)&cwEnabled);
    if (cwEnabled) {
        char* cwPtr = nullptr;
        GetAttr(MUIA_String_Contents, m_StringCW, (IPTR*)&cwPtr);
        spoilerText = cwPtr ? cwPtr : "";
    }
    
    // Post the status
    printf("DEBUG: Posting status with visibility: %s\n", visibility.c_str());
    m_API.PostStatus(content, visibility, spoilerText, [this](bool success, const std::string& statusId) {
        if (success) {
            printf("Post successful! Status ID: %s\n", statusId.c_str());
            
            // Clear the text editor
            // Try TextEditor method
            if (DoMethod(m_TextEditor, MUIM_TextEditor_ClearText) == 0) {
                // If that returned 0 (failed/not supported/no method), try setting String contents to empty
                SetAttrs(m_TextEditor, MUIA_String_Contents, (IPTR)"", TAG_DONE);
            }
            
            // Reset character counter
            SetAttrs(m_LabelCharCount, MUIA_Text_Contents, (IPTR)"500 characters remaining", TAG_DONE);
            
            // Refresh timeline to show new post
            FetchTimeline();
            
            // TODO: Show success message to user via MUI requester
        } else {
            printf("Post failed!\n");
            // TODO: Show error message to user via MUI requester
        }
    });
}

void MainWindow::SetAccountInfo(const std::string& username, const std::string& instance, const std::string& avatarPath) {
    if (m_LabelUsername) {
        SetAttrs(m_LabelUsername, MUIA_Text_Contents, (IPTR)username.c_str(), TAG_DONE);
    }
    if (m_LabelInstance) {
        SetAttrs(m_LabelInstance, MUIA_Text_Contents, (IPTR)instance.c_str(), TAG_DONE);
    }
    
    // Set avatar
    if (m_ImageAvatar && !avatarPath.empty()) {
        // Heap-allocated and intentionally never freed: avoids running a
        // std::string destructor during C++ global teardown.
        static std::string* s_avatarSpec = new std::string;
        *s_avatarSpec = "PROGDIR:" + avatarPath;

        if (m_UseGuigfx) {
            printf("DEBUG: Setting avatar spec (Guigfx): '%s'\n", s_avatarSpec->c_str());
            SetAttrs(m_ImageAvatar, MUIA_Guigfx_FileName, (IPTR)s_avatarSpec->c_str(), TAG_DONE);
        } else {
            printf("DEBUG: Setting avatar spec (Dtpic): '%s'\n", s_avatarSpec->c_str());
            SetAttrs(m_ImageAvatar, MUIA_Dtpic_Name, (IPTR)s_avatarSpec->c_str(), TAG_DONE);
        }
    }
    
    printf("DEBUG: UI updated with username: %s, instance: %s\n", username.c_str(), instance.c_str());
}

void MainWindow::UpdateCharacterCounter() {
    if (!m_TextEditor || !m_LabelCharCount) return;
    
    // Get current text length
    char* contentPtr = nullptr;
    DoMethod(m_TextEditor, MUIM_TextEditor_ExportText, &contentPtr);
    std::string content = contentPtr ? contentPtr : "";
    
    int remaining = 500 - content.length();
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%d characters remaining", remaining);
    
    SetAttrs(m_LabelCharCount, MUIA_Text_Contents, (IPTR)buffer, TAG_DONE);
}

void MainWindow::ShowToot(int index) {
    if (index < 0 || index >= (int)m_TimelineData.size()) return;
    if (!m_Htmlview) return;

    const Status& status = m_TimelineData[index];

    // Heap-allocated and intentionally never freed: avoids running a
    // std::string destructor during C++ global teardown.
    static std::string* s_html = new std::string;
    *s_html = TootFormatter::FormatToot(status);

    SetAttrs(m_Htmlview, MUIA_HTMLview_Contents, (IPTR)s_html->c_str(), TAG_DONE);
}

Object* MainWindow::CreateNotificationsPage() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"DUMMY NOTIFICATIONS", TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, TAG_DONE),
        TAG_DONE);
}

Object* MainWindow::CreateExplorePage() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"DUMMY EXPLORE", TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, TAG_DONE),
        TAG_DONE);
}

Object* MainWindow::CreateDMSPage() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"DUMMY DMs", TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, TAG_DONE),
        TAG_DONE);
}

Object* MainWindow::CreateFavouritesPage() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"DUMMY FAVOURITES", TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, TAG_DONE),
        TAG_DONE);
}

Object* MainWindow::CreateBookmarksPage() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"DUMMY BOOKMARKS", TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, TAG_DONE),
        TAG_DONE);
}

Object* MainWindow::CreateListsPage() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"DUMMY LISTS", TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, TAG_DONE),
        TAG_DONE);
}

Object* MainWindow::CreateRequestsPage() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"DUMMY REQUESTS", TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, TAG_DONE),
        TAG_DONE);
}

Object* MainWindow::CreateProfilePage() {
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"DUMMY PROFILE", TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, TAG_DONE),
        TAG_DONE);
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
    if (!app || !m_Window) {
        printf("ERROR: MainWindow::InitNotifications called with NULL app or m_Window!\n");
        return;
    }
    
    // Quit
    if (m_ItemQuit) {
        DoMethod(m_ItemQuit, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, 
            app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    }
        
    // Settings
    if (m_ItemSettings) {
        DoMethod(m_ItemSettings, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            app, 2, MUIM_Application_ReturnID, APPRETURN_SETTINGS);
    }
        
    // MUI Settings
    if (m_ItemMUI) {
        DoMethod(m_ItemMUI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
            app, 2, MUIM_Application_OpenConfigWindow, 0);
    }
        
    // Timeline Refresh
    if (m_BtnRefreshTimeline) {
        DoMethod(m_BtnRefreshTimeline, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, APPRETURN_REFRESH_TIMELINE);
    }
    
    // Post Button
    if (m_BtnPost) {
        DoMethod(m_BtnPost, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, APPRETURN_POST);
    }
    
    // Content Warning Checkbox - Enable/disable CW text field
    if (m_CheckCW && m_StringCW) {
        DoMethod(m_CheckCW, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            m_StringCW, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);
    }
    
    // TextEditor - Update character counter on every change
    if (m_TextEditor && m_LabelCharCount) {
        // Only if it's the actual MCC (check if we can differentiate or just try)
        // For now, simpler: just try adding notification, if not supported it might fail silently
        // But better is to check if we shouldn't.
        // If m_TextEditor is String class, it has MUIA_String_Contents
        // TextEditor.mcc uses MUIM_TextEditor_HasChanged
        
        // We can't easily check class type at runtime without RTTI or MUI introspection
        // But we know if the MCC failed, we created a String object.
        // Let's rely on the user seeing "TextEditor.mcc not found"
        
        // Actually, safer to only add this notification if it really IS a texteditor
        // But we don't have a flag.
        
        // TRY adding it. If object doesn't support method, MUI usually ignores it.
         DoMethod(m_TextEditor, MUIM_Notify, MUIA_TextEditor_HasChanged, MUIV_EveryTime,
                 app, 2, MUIM_Application_ReturnID, APPRETURN_TEXT_CHANGED);
    }

    if (m_ListTimelineInner) {
        DoMethod(m_ListTimelineInner, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
            app, 2, MUIM_Application_ReturnID, APPRETURN_TIMELINE_SELECT);
    }
}

Object* MainWindow::Create() {
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

    Object* menu = CreateMenu();

    m_ListNavigation = MUIHelpers_NewObject(MUIC_Listview,
                        MUIA_Weight, 20,
                        MUIA_Listview_List, MUIHelpers_NewObject(MUIC_List,
                            MUIA_List_SourceArray, (IPTR)titles,
                            TAG_DONE),
                        TAG_DONE);

    Object* pPublish = CreatePublishPage();
    Object* pTimeline = CreateTimelinePage();

    m_PageGroup = MUIHelpers_NewObject(MUIC_Group,
                    MUIA_Weight, 80, 
                    MUIA_Group_PageMode, TRUE,
                    MUIA_Group_Child, (IPTR)pPublish,
                    MUIA_Group_Child, (IPTR)CreateNotificationsPage(),
                    MUIA_Group_Child, (IPTR)CreateExplorePage(),
                    MUIA_Group_Child, (IPTR)pTimeline,
                    MUIA_Group_Child, (IPTR)CreateDMSPage(),
                    MUIA_Group_Child, (IPTR)CreateFavouritesPage(),
                    MUIA_Group_Child, (IPTR)CreateBookmarksPage(),
                    MUIA_Group_Child, (IPTR)CreateListsPage(),
                    MUIA_Group_Child, (IPTR)CreateRequestsPage(),
                    MUIA_Group_Child, (IPTR)CreateProfilePage(),
                    TAG_DONE);

    // 3. Balance bar
    Object* balance = MUIHelpers_NewObject(MUIC_Balance,
                        MUIA_CycleChain, 1,
                        MUIA_ObjectID, MAKE_ID('B','1','2','8'),
                        TAG_DONE);

    // 4. Horizontal container: Nav (20) | Balance | PageGroup (80)
    Object* horizGroup = MUIHelpers_NewObject(MUIC_Group,
                        MUIA_Group_Horiz, TRUE,
                        MUIA_Group_Spacing, 0,
                        MUIA_Group_Child, (IPTR)m_ListNavigation,
                        MUIA_Group_Child, (IPTR)balance,
                        MUIA_Group_Child, (IPTR)m_PageGroup,
                        TAG_DONE);

    // 5. Root group - Just the horizontal layout for now
    Object* rootGroup = MUIHelpers_NewObject(MUIC_Group,
                        MUIA_Group_Horiz, FALSE,
                        MUIA_Group_Spacing, 0,
                        MUIA_Group_Child, (IPTR)horizGroup,
                        TAG_DONE);

    m_Window = MUIHelpers_NewObject(MUIC_Window,
        MUIA_Window_Title, (IPTR)"Amidon2 (Layout 1.28 - Restore)",
        MUIA_Window_ID, 0x52455354, // 'REST'
        MUIA_Window_ScreenTitle, (IPTR)"Amidon2 - Mastodon Client",
        MUIA_Window_AppWindow, TRUE,
        MUIA_Window_Width, 600,
        MUIA_Window_Height, 400,
        MUIA_Window_SizeGadget, TRUE,
        MUIA_Window_SizeRight, TRUE,
        MUIA_Window_CloseGadget, TRUE,
        MUIA_Window_DepthGadget, TRUE,
        MUIA_Window_Menustrip, (IPTR)menu,
        MUIA_Window_RootObject, (IPTR)rootGroup,
        TAG_DONE
    );


    if (m_Window && m_ListNavigation && m_PageGroup) {
        Object* navList;
        GetAttr(MUIA_Listview_List, m_ListNavigation, (ULONG*)&navList);
        if (navList) {
            DoMethod(navList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
                m_PageGroup, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue);
        }
    }

    return m_Window;
}

