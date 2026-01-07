#include "LoginDialog.h"
#include "MUIHelpers.h"

#include <proto/muimaster.h>
#include <libraries/mui.h>

LoginDialog::LoginDialog() : m_Dialog(nullptr) {
}

LoginDialog::~LoginDialog() {
}

#include "../Constants.h"
 
Object* LoginDialog::Create() {
    m_Dialog = MUIHelpers_NewObject(MUIC_Window,
        MUIA_Window_Title, (IPTR)"Login to Mastodon",
        MUIA_Window_ID, 0x4C4F4749, // 'LOGI'
        MUIA_Window_RootObject, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Horiz, FALSE,
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, 
                MUIA_Text_Contents, (IPTR)"1. Visit this URL to authorize the app:", 
                TAG_DONE),
            MUIA_Group_Child, (IPTR)(m_StrAuthURL = MUIHelpers_NewObject(MUIC_String,
                MUIA_Frame, MUIV_Frame_String,
                MUIA_String_Contents, (IPTR)"(Registering app...)",
                MUIA_Disabled, TRUE,
                TAG_DONE)),
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, 
                MUIA_Text_Contents, (IPTR)"2. Copy the authorization code and paste it below:", 
                TAG_DONE),
            MUIA_Group_Child, (IPTR)(m_StrCode = MUIHelpers_NewObject(MUIC_String,
                MUIA_Frame, MUIV_Frame_String,
                MUIA_ControlChar, 'p',
                TAG_DONE)),
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Group,
                MUIA_Group_Horiz, TRUE,
                MUIA_Group_Child, (IPTR)(m_BtnLogin = MUIHelpers_NewObject(MUIC_Text,
                    MUIA_Frame, MUIV_Frame_Button,
                    MUIA_Background, MUII_ButtonBack,
                    MUIA_Text_Contents, (IPTR)"Login",
                    MUIA_Text_PreParse, (IPTR)"\33c",
                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                    MUIA_ControlChar, 'l',
                    TAG_DONE)),
                MUIA_Group_Child, (IPTR)(m_BtnCancel = MUIHelpers_NewObject(MUIC_Text,
                    MUIA_Frame, MUIV_Frame_Button,
                    MUIA_Background, MUII_ButtonBack,
                    MUIA_Text_Contents, (IPTR)"Cancel",
                    MUIA_Text_PreParse, (IPTR)"\33c",
                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                    MUIA_ControlChar, 'c',
                    TAG_DONE)),
                TAG_DONE),
            TAG_DONE),
        TAG_DONE);
    return m_Dialog;
}
 
void LoginDialog::InitNotifications(Object* app) {
    if (!app) return;
    
    // Window Close -> Just hide
    DoMethod(m_Dialog, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        m_Dialog, 3, MUIM_Set, MUIA_Window_Open, FALSE);
        
    // Cancel Button -> Hide
    DoMethod(m_BtnCancel, MUIM_Notify, MUIA_Pressed, FALSE,
        m_Dialog, 3, MUIM_Set, MUIA_Window_Open, FALSE);
        
    // Login Button -> Return APPRETURN_DO_LOGIN
    DoMethod(m_BtnLogin, MUIM_Notify, MUIA_Pressed, FALSE,
        app, 2, MUIM_Application_ReturnID, APPRETURN_DO_LOGIN);
}
 
void LoginDialog::SetAuthURL(const char* url) {
    if (m_StrAuthURL) {
        SetAttrs(m_StrAuthURL, MUIA_String_Contents, (IPTR)url, TAG_DONE);
    }
}
 
std::string LoginDialog::GetAuthCode() {
    char *code = nullptr;
    if (m_StrCode) {
        GetAttr(MUIA_String_Contents, m_StrCode, (IPTR*)&code);
    }
    return code ? std::string(code) : "";
}
