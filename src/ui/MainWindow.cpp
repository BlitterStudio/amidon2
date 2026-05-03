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

MainWindow::MainWindow(MastodonAPI& api)
    : m_Window(nullptr),
      m_ListTimeline(nullptr), m_ListTimelineInner(nullptr), m_ListNavigation(nullptr),
      m_ListNavigationInner(nullptr), m_PageGroup(nullptr),
      m_BtnRefreshTimeline(nullptr),
      m_BtnFavouriteToot(nullptr), m_BtnBoostToot(nullptr),
      m_BtnRefreshNotifications(nullptr),
      m_ListNotifications(nullptr), m_ListNotificationsInner(nullptr),
      m_BtnRefreshFavourites(nullptr),
      m_ListFavourites(nullptr), m_ListFavouritesInner(nullptr), m_HtmlviewFavourites(nullptr),
      m_BtnRefreshBookmarks(nullptr),
      m_ListBookmarks(nullptr), m_ListBookmarksInner(nullptr), m_HtmlviewBookmarks(nullptr),
      m_LabelProfileDisplay(nullptr), m_LabelProfileAcct(nullptr),
      m_LabelProfileStats(nullptr), m_HtmlviewProfileBio(nullptr),
      m_LabelUsername(nullptr), m_LabelInstance(nullptr),
      m_ImageAvatar(nullptr), m_HeaderGroup(nullptr),
      m_TextEditor(nullptr), m_LabelCharCount(nullptr),
      m_CycleVisibility(nullptr), m_CheckCW(nullptr), m_StringCW(nullptr), m_BtnPost(nullptr),
      m_Htmlview(nullptr), m_HtmlviewNotifications(nullptr), m_NetHook({}),
      m_ItemQuit(nullptr), m_ItemSettings(nullptr), m_ItemMUI(nullptr),
      m_API(api), m_App(nullptr), m_UseGuigfx(false)  // set in CreateHeader
{
    HTMLviewNet_InitHook(&m_NetHook);
}

Object* MainWindow::CreateHeader() {
    Object* avatarHolder = nullptr;

    // Try Guigfx.mcc first — it scales the image to the widget size, so
    // a plain fixed-size widget gives a properly resized avatar.
    m_ImageAvatar = MUIHelpers_NewObject((char*)MUIC_Guigfx,
        MUIA_Guigfx_ScaleMode, (IPTR)(NISMF_SCALEFREE | NISMF_KEEPASPECT_PICTURE),
        MUIA_Guigfx_Quality, (IPTR)MUIV_Guigfx_Quality_Low,
        MUIA_FixWidth, 48,
        MUIA_FixHeight, 48,
        TAG_DONE);

    if (m_ImageAvatar) {
        fprintf(stderr, "DEBUG: Guigfx.mcc loaded; avatar will be scaled\n");
        m_UseGuigfx = true;
        avatarHolder = m_ImageAvatar;
    } else {
        fprintf(stderr, "DEBUG: Guigfx.mcc unavailable; falling back to Dtpic crop\n");
        m_UseGuigfx = false;

        // Dtpic renders the image at its native size and ignores
        // MUIA_FixWidth/FixHeight on the image itself, so wrap it in a
        // size-constrained Scrollgroup that clips the rendered image to
        // a 48x48 viewport. Without proper scaling this shows the top-left
        // of the avatar — better than letting it eat the window.
        m_ImageAvatar = MUIHelpers_NewObject(MUIC_Dtpic,
            MUIA_Dtpic_Name, (IPTR)"",
            TAG_DONE);

        avatarHolder = MUIHelpers_NewObject(MUIC_Scrollgroup,
            MUIA_Scrollgroup_Contents, (IPTR)MUIHelpers_NewObject(MUIC_Virtgroup,
                MUIA_Group_Child, (IPTR)m_ImageAvatar,
                TAG_DONE),
            MUIA_Scrollgroup_NoHorizBar, TRUE,
            MUIA_Scrollgroup_NoVertBar, TRUE,
            MUIA_FixWidth, 48,
            MUIA_FixHeight, 48,
            TAG_DONE);
    }

    m_HeaderGroup = MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Child, (IPTR)avatarHolder,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, FALSE,
            MUIA_Group_Child, (IPTR)(m_LabelUsername = MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"Username", TAG_DONE)),
            MUIA_Group_Child, (IPTR)(m_LabelInstance = MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"Instance", TAG_DONE)),
            TAG_DONE),
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, TAG_DONE), // Spacer
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, MUIA_FixWidth, 32, MUIA_FixHeight, 32, TAG_DONE), // Placeholder for Logo
        TAG_DONE);

    return m_HeaderGroup;
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

    Object* actions = MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Child, (IPTR)(m_BtnFavouriteToot = (Object*)MUIHelpers_NewObject(MUIC_Text,
            MUIA_Frame, MUIV_Frame_Button,
            MUIA_Background, MUII_ButtonBack,
            MUIA_InputMode, MUIV_InputMode_RelVerify,
            MUIA_Text_Contents, (IPTR)"\33cFavourite",
            TAG_DONE)),
        MUIA_Group_Child, (IPTR)(m_BtnBoostToot = (Object*)MUIHelpers_NewObject(MUIC_Text,
            MUIA_Frame, MUIV_Frame_Button,
            MUIA_Background, MUII_ButtonBack,
            MUIA_InputMode, MUIV_InputMode_RelVerify,
            MUIA_Text_Contents, (IPTR)"\33cBoost",
            TAG_DONE)),
        TAG_DONE);

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
        MUIA_Group_Child, (IPTR)actions,
        TAG_DONE);

    if (!page) printf("FATAL: CreateTimelinePage failed!\n");
    return page;
}

void MainWindow::HandleFavouriteToot() {
    LONG active = MUIV_List_Active_Off;
    if (m_ListTimelineInner) {
        GetAttr(MUIA_List_Active, m_ListTimelineInner, (IPTR*)&active);
    }
    if (active == MUIV_List_Active_Off) return;
    if (active < 0 || active >= (LONG)m_TimelineData.size()) return;

    Status& status = m_TimelineData[active];
    const std::string id = status.id;
    if (id.empty()) return;

    bool wasFavourited = status.favourited;
    fprintf(stderr, "DEBUG: HandleFavouriteToot id=%s currently=%s\n",
            id.c_str(), wasFavourited ? "yes" : "no");

    auto onDone = [this, id, wasFavourited](bool success) {
        fprintf(stderr, "DEBUG: %s %s -> %s\n",
                wasFavourited ? "Unfavourite" : "Favourite",
                id.c_str(), success ? "ok" : "failed");
        if (!success) return;

        // Find the row again — index may have shifted if a refresh ran.
        for (size_t i = 0; i < m_TimelineData.size(); i++) {
            if (m_TimelineData[i].id == id) {
                m_TimelineData[i].favourited = !wasFavourited;
                LONG cur = MUIV_List_Active_Off;
                if (m_ListTimelineInner) {
                    GetAttr(MUIA_List_Active, m_ListTimelineInner, (IPTR*)&cur);
                }
                if ((LONG)i == cur && m_BtnFavouriteToot) {
                    SetAttrs(m_BtnFavouriteToot, MUIA_Text_Contents,
                             (IPTR)(!wasFavourited ? "\33cFavourited" : "\33cFavourite"),
                             TAG_DONE);
                }
                break;
            }
        }
    };

    if (wasFavourited) m_API.UnfavouriteStatus(id, onDone);
    else                m_API.FavouriteStatus(id, onDone);
}

void MainWindow::HandleBoostToot() {
    LONG active = MUIV_List_Active_Off;
    if (m_ListTimelineInner) {
        GetAttr(MUIA_List_Active, m_ListTimelineInner, (IPTR*)&active);
    }
    if (active == MUIV_List_Active_Off) return;
    if (active < 0 || active >= (LONG)m_TimelineData.size()) return;

    Status& status = m_TimelineData[active];
    const std::string id = status.id;
    if (id.empty()) return;

    bool wasReblogged = status.reblogged;
    fprintf(stderr, "DEBUG: HandleBoostToot id=%s currently=%s\n",
            id.c_str(), wasReblogged ? "yes" : "no");

    auto onDone = [this, id, wasReblogged](bool success) {
        fprintf(stderr, "DEBUG: %s %s -> %s\n",
                wasReblogged ? "Unboost" : "Boost",
                id.c_str(), success ? "ok" : "failed");
        if (!success) return;

        for (size_t i = 0; i < m_TimelineData.size(); i++) {
            if (m_TimelineData[i].id == id) {
                m_TimelineData[i].reblogged = !wasReblogged;
                LONG cur = MUIV_List_Active_Off;
                if (m_ListTimelineInner) {
                    GetAttr(MUIA_List_Active, m_ListTimelineInner, (IPTR*)&cur);
                }
                if ((LONG)i == cur && m_BtnBoostToot) {
                    SetAttrs(m_BtnBoostToot, MUIA_Text_Contents,
                             (IPTR)(!wasReblogged ? "\33cBoosted" : "\33cBoost"),
                             TAG_DONE);
                }
                break;
            }
        }
    };

    if (wasReblogged) m_API.UnreblogStatus(id, onDone);
    else               m_API.ReblogStatus(id, onDone);
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

        // Wrap the dynamic image-name change in InitChange/ExitChange on
        // the parent group so MUI / Dtpic relayout and reload the image
        // (Dtpic loads from datatypes at MUIM_Setup; without InitChange
        // the new image often doesn't appear).
        if (m_HeaderGroup) DoMethod(m_HeaderGroup, MUIM_Group_InitChange);

        if (m_UseGuigfx) {
            printf("DEBUG: Setting avatar spec (Guigfx): '%s'\n", s_avatarSpec->c_str());
            SetAttrs(m_ImageAvatar, MUIA_Guigfx_FileName, (IPTR)s_avatarSpec->c_str(), TAG_DONE);
        } else {
            printf("DEBUG: Setting avatar spec (Dtpic): '%s'\n", s_avatarSpec->c_str());
            SetAttrs(m_ImageAvatar, MUIA_Dtpic_Name, (IPTR)s_avatarSpec->c_str(), TAG_DONE);
        }

        if (m_HeaderGroup) DoMethod(m_HeaderGroup, MUIM_Group_ExitChange);
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

    // Reflect favourited / reblogged state on the action buttons.
    if (m_BtnFavouriteToot) {
        SetAttrs(m_BtnFavouriteToot, MUIA_Text_Contents,
                 (IPTR)(status.favourited ? "\33cFavourited" : "\33cFavourite"),
                 TAG_DONE);
    }
    if (m_BtnBoostToot) {
        SetAttrs(m_BtnBoostToot, MUIA_Text_Contents,
                 (IPTR)(status.reblogged ? "\33cBoosted" : "\33cBoost"),
                 TAG_DONE);
    }
}

Object* MainWindow::CreateNotificationsPage() {
    m_ListNotificationsInner = MUIHelpers_NewObject(MUIC_List, TAG_DONE);
    if (!m_ListNotificationsInner) {
        printf("FATAL: Failed to create m_ListNotificationsInner!\n");
        return nullptr;
    }

    Object* htmlScrollgroup = MUIHelpers_NewObject(MUIC_Scrollgroup,
        MUIA_Scrollgroup_FreeVert, TRUE,
        MUIA_Scrollgroup_FreeHoriz, TRUE,
        MUIA_Scrollgroup_Contents, m_HtmlviewNotifications = MUIHelpers_NewObject(MUIC_HTMLview,
            MUIA_Background, MUII_TextBack,
            MUIA_HTMLview_ImageLoadHook, (IPTR)&m_NetHook,
            MUIA_HTMLview_LoadHook, (IPTR)&m_NetHook,
            MUIA_HTMLview_Contents, (IPTR)"",
            TAG_DONE),
        TAG_DONE);

    Object* bottom = (htmlScrollgroup && m_HtmlviewNotifications)
                     ? htmlScrollgroup
                     : MUIHelpers_NewObject(MUIC_Rectangle, MUIA_Frame, MUIV_Frame_None, TAG_DONE);

    Object* page = MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)(m_BtnRefreshNotifications = (Object*)MUIHelpers_NewObject(MUIC_Text,
            MUIA_Frame, MUIV_Frame_Button,
            MUIA_Background, MUII_ButtonBack,
            MUIA_InputMode, MUIV_InputMode_RelVerify,
            MUIA_Text_Contents, (IPTR)"\33cRefresh Notifications",
            TAG_DONE)),
        MUIA_Group_Child, (IPTR)(m_ListNotifications = MUIHelpers_NewObject(MUIC_Listview,
            MUIA_Weight, 50,
            MUIA_Listview_List, (IPTR)m_ListNotificationsInner,
            TAG_DONE)),
        MUIA_Group_Child, (IPTR)bottom,
        TAG_DONE);

    if (!page) printf("FATAL: CreateNotificationsPage failed!\n");
    return page;
}

void MainWindow::ShowNotification(int index) {
    if (index < 0 || index >= (int)m_NotificationData.size()) return;
    if (!m_HtmlviewNotifications) return;

    const Notification& n = m_NotificationData[index];

    std::string verb;
    if (n.type == "follow")            verb = "followed you";
    else if (n.type == "favourite")    verb = "favourited your post";
    else if (n.type == "reblog")       verb = "boosted your post";
    else if (n.type == "mention")      verb = "mentioned you";
    else if (n.type == "poll")         verb = "ended a poll";
    else if (n.type == "status")       verb = "posted";
    else if (n.type == "update")       verb = "edited a post";
    else                                verb = n.type;

    std::string display = n.author_displayname.empty() ? n.author_username : n.author_displayname;

    // Heap-allocated and intentionally never freed: avoids running a
    // std::string destructor during C++ global teardown.
    //
    // NOTE: avatar URL is stored on the Notification but not embedded here
    // because HTMLview.mcc doesn't scale <img width/height> — full-size
    // avatars overwhelm the pane. Re-introduce when we have proper scaling.
    static std::string* s_html = new std::string;
    *s_html = "<html><body>";
    *s_html += "<b>" + display + "</b> <i>@" + n.author_username + "</i><br>";
    *s_html += verb;
    if (!n.status_excerpt.empty()) {
        *s_html += "<br><br>";
        *s_html += n.status_excerpt;
    }
    *s_html += "</body></html>";

    SetAttrs(m_HtmlviewNotifications, MUIA_HTMLview_Contents, (IPTR)s_html->c_str(), TAG_DONE);
}

void MainWindow::FetchNotifications() {
    m_API.GetNotifications([this](std::vector<Notification> notifs) {
        if (!m_ListNotificationsInner) return;

        DoMethod(m_ListNotificationsInner, MUIM_List_Clear);
        m_NotificationData = notifs;
        m_NotificationStrings.clear();

        for (const auto& n : notifs) {
            std::string verb;
            if (n.type == "follow")            verb = "followed you";
            else if (n.type == "favourite")    verb = "favourited";
            else if (n.type == "reblog")       verb = "boosted";
            else if (n.type == "mention")      verb = "mentioned you";
            else if (n.type == "poll")         verb = "ended a poll";
            else if (n.type == "status")       verb = "posted";
            else if (n.type == "update")       verb = "edited";
            else                                verb = n.type;

            std::string who = n.author_displayname.empty() ? n.author_username : n.author_displayname;
            std::string line = "@" + who + " " + verb;
            if (!n.status_excerpt.empty()) {
                std::string excerpt = n.status_excerpt;
                if (excerpt.length() > 80) excerpt = excerpt.substr(0, 77) + "...";
                line += ": " + excerpt;
            }

            m_NotificationStrings.push_back(line);
            DoMethod(m_ListNotificationsInner, MUIM_List_InsertSingle,
                     (IPTR)m_NotificationStrings.back().c_str(), MUIV_List_Insert_Bottom);
        }
    });
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

namespace {

// Build a [refresh-button | listview | htmlview] page for a status list,
// returning the page Object* and writing the created widgets back through
// the provided pointers. Mirrors CreateTimelinePage's structure.
Object* BuildStatusListPage(const char* refreshLabel,
                             struct Hook* netHook,
                             Object*& btnRefresh,
                             Object*& listInner,
                             Object*& list,
                             Object*& htmlview) {
    listInner = MUIHelpers_NewObject(MUIC_List, TAG_DONE);
    if (!listInner) return nullptr;

    Object* htmlScrollgroup = MUIHelpers_NewObject(MUIC_Scrollgroup,
        MUIA_Scrollgroup_FreeVert, TRUE,
        MUIA_Scrollgroup_FreeHoriz, TRUE,
        MUIA_Scrollgroup_Contents, htmlview = MUIHelpers_NewObject(MUIC_HTMLview,
            MUIA_Background, MUII_TextBack,
            MUIA_HTMLview_ImageLoadHook, (IPTR)netHook,
            MUIA_HTMLview_LoadHook, (IPTR)netHook,
            MUIA_HTMLview_Contents, (IPTR)"",
            TAG_DONE),
        TAG_DONE);

    Object* bottom = (htmlScrollgroup && htmlview)
                     ? htmlScrollgroup
                     : MUIHelpers_NewObject(MUIC_Rectangle, MUIA_Frame, MUIV_Frame_None, TAG_DONE);

    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)(btnRefresh = (Object*)MUIHelpers_NewObject(MUIC_Text,
            MUIA_Frame, MUIV_Frame_Button,
            MUIA_Background, MUII_ButtonBack,
            MUIA_InputMode, MUIV_InputMode_RelVerify,
            MUIA_Text_Contents, (IPTR)refreshLabel,
            TAG_DONE)),
        MUIA_Group_Child, (IPTR)(list = MUIHelpers_NewObject(MUIC_Listview,
            MUIA_Weight, 50,
            MUIA_Listview_List, (IPTR)listInner,
            TAG_DONE)),
        MUIA_Group_Child, (IPTR)bottom,
        TAG_DONE);
}

// Render a status array into an MUI list, keeping per-row strings alive
// in `strings`. Clears both before populating.
void PopulateStatusList(Object* listInner,
                        const std::vector<Status>& statuses,
                        std::list<std::string>& strings,
                        std::vector<Status>& dataSink) {
    if (!listInner) return;
    DoMethod(listInner, MUIM_List_Clear);
    dataSink = statuses;
    strings.clear();
    for (const auto& s : statuses) {
        strings.push_back(s.author_username + ": " + s.content);
        DoMethod(listInner, MUIM_List_InsertSingle,
                 (IPTR)strings.back().c_str(), MUIV_List_Insert_Bottom);
    }
}

void RenderStatusToHtmlview(Object* htmlview, std::string* sink, const Status& s) {
    if (!htmlview || !sink) return;
    *sink = TootFormatter::FormatToot(s);
    SetAttrs(htmlview, MUIA_HTMLview_Contents, (IPTR)sink->c_str(), TAG_DONE);
}

}  // namespace

Object* MainWindow::CreateFavouritesPage() {
    return BuildStatusListPage("\33cRefresh Favourites",
                                &m_NetHook,
                                m_BtnRefreshFavourites,
                                m_ListFavouritesInner,
                                m_ListFavourites,
                                m_HtmlviewFavourites);
}

Object* MainWindow::CreateBookmarksPage() {
    return BuildStatusListPage("\33cRefresh Bookmarks",
                                &m_NetHook,
                                m_BtnRefreshBookmarks,
                                m_ListBookmarksInner,
                                m_ListBookmarks,
                                m_HtmlviewBookmarks);
}

void MainWindow::FetchFavourites() {
    m_API.GetFavourites([this](std::vector<Status> data) {
        PopulateStatusList(m_ListFavouritesInner, data, m_FavouritesStrings, m_FavouritesData);
    });
}

void MainWindow::FetchBookmarks() {
    m_API.GetBookmarks([this](std::vector<Status> data) {
        PopulateStatusList(m_ListBookmarksInner, data, m_BookmarksStrings, m_BookmarksData);
    });
}

void MainWindow::ShowFavourite(int index) {
    if (index < 0 || index >= (int)m_FavouritesData.size()) return;
    static std::string* s_html = new std::string;
    RenderStatusToHtmlview(m_HtmlviewFavourites, s_html, m_FavouritesData[index]);
}

void MainWindow::ShowBookmark(int index) {
    if (index < 0 || index >= (int)m_BookmarksData.size()) return;
    static std::string* s_html = new std::string;
    RenderStatusToHtmlview(m_HtmlviewBookmarks, s_html, m_BookmarksData[index]);
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
    Object* bioScroll = MUIHelpers_NewObject(MUIC_Scrollgroup,
        MUIA_Scrollgroup_FreeVert, TRUE,
        MUIA_Scrollgroup_FreeHoriz, TRUE,
        MUIA_Scrollgroup_Contents, m_HtmlviewProfileBio = MUIHelpers_NewObject(MUIC_HTMLview,
            MUIA_Background, MUII_TextBack,
            MUIA_HTMLview_ImageLoadHook, (IPTR)&m_NetHook,
            MUIA_HTMLview_LoadHook, (IPTR)&m_NetHook,
            MUIA_HTMLview_Contents, (IPTR)"",
            TAG_DONE),
        TAG_DONE);

    Object* bottom = (bioScroll && m_HtmlviewProfileBio)
                     ? bioScroll
                     : MUIHelpers_NewObject(MUIC_Rectangle, MUIA_Frame, MUIV_Frame_None, TAG_DONE);

    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_Child, (IPTR)(m_LabelProfileDisplay = MUIHelpers_NewObject(MUIC_Text,
            MUIA_Text_Contents, (IPTR)"\33b(not signed in)",
            TAG_DONE)),
        MUIA_Group_Child, (IPTR)(m_LabelProfileAcct = MUIHelpers_NewObject(MUIC_Text,
            MUIA_Text_Contents, (IPTR)"",
            TAG_DONE)),
        MUIA_Group_Child, (IPTR)(m_LabelProfileStats = MUIHelpers_NewObject(MUIC_Text,
            MUIA_Text_Contents, (IPTR)"",
            TAG_DONE)),
        MUIA_Group_Child, (IPTR)bottom,
        TAG_DONE);
}

void MainWindow::SetProfile(const Account& account) {
    if (m_LabelProfileDisplay) {
        // Heap-allocated to keep the c_str() pointer alive for MUI without
        // triggering a destructor at C++ global teardown.
        static std::string* s_display = new std::string;
        *s_display = "\33b" + (account.display_name.empty() ? account.username : account.display_name);
        SetAttrs(m_LabelProfileDisplay, MUIA_Text_Contents, (IPTR)s_display->c_str(), TAG_DONE);
    }

    if (m_LabelProfileAcct) {
        static std::string* s_acct = new std::string;
        *s_acct = "@" + account.acct;
        SetAttrs(m_LabelProfileAcct, MUIA_Text_Contents, (IPTR)s_acct->c_str(), TAG_DONE);
    }

    if (m_LabelProfileStats) {
        static std::string* s_stats = new std::string;
        char buf[160];
        snprintf(buf, sizeof(buf), "%d following  -  %d followers  -  %d toots",
                 account.following_count, account.followers_count, account.statuses_count);
        *s_stats = buf;
        SetAttrs(m_LabelProfileStats, MUIA_Text_Contents, (IPTR)s_stats->c_str(), TAG_DONE);
    }

    if (m_HtmlviewProfileBio) {
        static std::string* s_bio = new std::string;
        *s_bio = "<html><body>";
        if (account.note.empty()) {
            *s_bio += "<i>(no bio)</i>";
        } else {
            *s_bio += account.note;
        }
        *s_bio += "</body></html>";
        SetAttrs(m_HtmlviewProfileBio, MUIA_HTMLview_Contents, (IPTR)s_bio->c_str(), TAG_DONE);
    }
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

    if (m_ListNavigationInner) {
        DoMethod(m_ListNavigationInner, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
            app, 2, MUIM_Application_ReturnID, APPRETURN_NAVIGATION);
    }

    if (m_BtnRefreshNotifications) {
        DoMethod(m_BtnRefreshNotifications, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, APPRETURN_REFRESH_NOTIFICATIONS);
    }

    if (m_ListNotificationsInner) {
        DoMethod(m_ListNotificationsInner, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
            app, 2, MUIM_Application_ReturnID, APPRETURN_NOTIFICATION_SELECT);
    }

    if (m_BtnRefreshFavourites) {
        DoMethod(m_BtnRefreshFavourites, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, APPRETURN_REFRESH_FAVOURITES);
    }
    if (m_ListFavouritesInner) {
        DoMethod(m_ListFavouritesInner, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
            app, 2, MUIM_Application_ReturnID, APPRETURN_FAVOURITES_SELECT);
    }

    if (m_BtnRefreshBookmarks) {
        DoMethod(m_BtnRefreshBookmarks, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, APPRETURN_REFRESH_BOOKMARKS);
    }
    if (m_ListBookmarksInner) {
        DoMethod(m_ListBookmarksInner, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
            app, 2, MUIM_Application_ReturnID, APPRETURN_BOOKMARKS_SELECT);
    }

    if (m_BtnFavouriteToot) {
        DoMethod(m_BtnFavouriteToot, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, APPRETURN_FAVOURITE_TOOT);
    }
    if (m_BtnBoostToot) {
        DoMethod(m_BtnBoostToot, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, APPRETURN_BOOST_TOOT);
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

    m_ListNavigationInner = MUIHelpers_NewObject(MUIC_List,
                                MUIA_List_SourceArray, (IPTR)titles,
                                TAG_DONE);

    m_ListNavigation = MUIHelpers_NewObject(MUIC_Listview,
                        MUIA_Weight, 20,
                        MUIA_Listview_List, (IPTR)m_ListNavigationInner,
                        TAG_DONE);

    Object* pPublish = CreatePublishPage();
    Object* pTimeline = CreateTimelinePage();
    Object* header = CreateHeader();

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

    // 5. Root group: header on top, then the horizontal nav | balance | page area.
    Object* rootGroup = MUIHelpers_NewObject(MUIC_Group,
                        MUIA_Group_Horiz, FALSE,
                        MUIA_Group_Spacing, 0,
                        MUIA_Group_Child, (IPTR)header,
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

