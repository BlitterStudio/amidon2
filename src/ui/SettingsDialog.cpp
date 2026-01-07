#include "SettingsDialog.h"
#include "MUIHelpers.h"
#include "../Constants.h"

#include <proto/muimaster.h>
#include <libraries/mui.h>

SettingsDialog::SettingsDialog() : m_Dialog(nullptr) {
}

SettingsDialog::~SettingsDialog() {
}

Object* SettingsDialog::Create() {
    m_Dialog = MUIHelpers_NewObject(MUIC_Window,
        MUIA_Window_Title, (IPTR)"Amidon Settings",
        MUIA_Window_ID, 0x53455454, // 'SETT'
        MUIA_Window_RootObject, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            
            // Mastodon Options Group
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
                MUIA_Frame, MUIV_Frame_Group,
                MUIA_FrameTitle, (IPTR)"Mastodon options",
                MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, 
                    MUIA_Text_Contents, (IPTR)"The Mastodon server to connect to", 
                    TAG_DONE),
                MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
                    MUIA_Group_Horiz, TRUE,
                    MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, MUIA_Text_Contents, (IPTR)"Server:", TAG_DONE),
                    MUIA_Group_Child, (IPTR)(m_StrInstance = MUIHelpers_NewObject(MUIC_String, 
                        MUIA_Frame, MUIV_Frame_String,
                        MUIA_String_Contents, (IPTR)"mastodon.social", 
                        TAG_DONE)),
                    TAG_DONE),
                TAG_DONE),

            // Login Group
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
                MUIA_Frame, MUIV_Frame_Group,
                MUIA_FrameTitle, (IPTR)"Login",
                MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, 
                    MUIA_Text_Contents, (IPTR)"You can login for your personalized feed,\nor browse the server anonymously", 
                    TAG_DONE),
                MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
                    MUIA_Group_Horiz, TRUE,
                    MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, TAG_DONE),
                    MUIA_Group_Child, (IPTR)(m_BtnLogin = MUIHelpers_NewObject(MUIC_Text,
                        MUIA_Frame, MUIV_Frame_Button,
                        MUIA_Background, MUII_ButtonBack,
                        MUIA_InputMode, MUIV_InputMode_RelVerify,
                        MUIA_Text_Contents, (IPTR)"\33cLogin",
                        TAG_DONE)),
                    TAG_DONE),
                TAG_DONE),

            // Buttons Group
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
                MUIA_Group_Horiz, TRUE,
                MUIA_Group_Child, (IPTR)(m_BtnSave = MUIHelpers_NewObject(MUIC_Text, 
                    MUIA_Frame, MUIV_Frame_Button,
                    MUIA_Background, MUII_ButtonBack,
                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                    MUIA_Text_Contents, (IPTR)"\33cSave Settings", 
                    TAG_DONE)),
                MUIA_Group_Child, (IPTR)(m_BtnCancel = MUIHelpers_NewObject(MUIC_Text, 
                    MUIA_Frame, MUIV_Frame_Button,
                    MUIA_Background, MUII_ButtonBack,
                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                    MUIA_Text_Contents, (IPTR)"\33cCancel", 
                    TAG_DONE)),
                TAG_DONE),
            TAG_DONE),
        TAG_DONE);
    return m_Dialog;
}

void SettingsDialog::InitNotifications(Object* app) {
    if (!app) return;
    
    DoMethod(m_Dialog, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        m_Dialog, 3, MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(m_BtnSave, MUIM_Notify, MUIA_Pressed, FALSE,
        app, 2, MUIM_Application_ReturnID, APPRETURN_SAVE_SETTINGS);
        
    DoMethod(m_BtnLogin, MUIM_Notify, MUIA_Pressed, FALSE,
        app, 2, MUIM_Application_ReturnID, APPRETURN_LOGIN);

    // Cancel just closes the window
    DoMethod(m_BtnCancel, MUIM_Notify, MUIA_Pressed, FALSE,
        m_Dialog, 3, MUIM_Set, MUIA_Window_Open, FALSE);
}

std::string SettingsDialog::GetInstanceURL() {
    char *url = nullptr;
    GetAttr(MUIA_String_Contents, m_StrInstance, (IPTR *)&url);
    if (url) {
        return std::string(url);
    }
    return "";
}
