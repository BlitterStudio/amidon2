#include "WelcomeDialog.h"
#include "MUIHelpers.h"
#include "../Constants.h"

#include <proto/muimaster.h>
#include <libraries/mui.h>

WelcomeDialog::WelcomeDialog() : m_Dialog(nullptr) {
}

WelcomeDialog::~WelcomeDialog() {
    // Dialog object is usually disposed by the application or parent window
    // if attached. If standalone, we might need to dispose it.
    // For now, let's assume App disposes it if it's part of the app object tree
    // or we dispose it manually if it's strictly modal and transient.
    // Given the pattern, if we create it, we own it until added to a parent.
}

Object* WelcomeDialog::Create() {
    
    // Original Text content
    const char* welcomeText = "Welcome to Amidon!\n\n"
                              "No existing settings were found, so you will need to provide\n"
                              "the Mastodon server instance you would like to connect to,\n"
                              "and you can optionally login as well. If you don't login,\n"
                              "you will only have access to the public information the\n"
                              "server provides.";

    m_Dialog = MUIHelpers_NewObject(MUIC_Window,
        MUIA_Window_Title, (IPTR)"Welcome to Amidon!",
        MUIA_Window_ID, 0x57454C43, // 'WELC'
        MUIA_Window_RootObject, (IPTR)MUIHelpers_NewObject(MUIC_Group,
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Dtpic,
                MUIA_Dtpic_Name, (IPTR)"PROGDIR:assets/Amidon_logo.png",
                TAG_DONE),
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Text, 
                MUIA_Text_Contents, (IPTR)welcomeText, 
                TAG_DONE),
            MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Rectangle, TAG_DONE),
            MUIA_Group_Child, (IPTR)(m_BtnOpenSettings = MUIHelpers_NewObject(MUIC_Text, 
                    MUIA_Frame, MUIV_Frame_Button,
                    MUIA_Background, MUII_ButtonBack,
                    MUIA_InputMode, MUIV_InputMode_RelVerify,
                    MUIA_Text_Contents, (IPTR)"\33cOpen Settings", 
                    TAG_DONE)),
            TAG_DONE),
        TAG_DONE);
        
    return m_Dialog;
}

void WelcomeDialog::InitNotifications(Object* app) {
    if (!app) return;
    
    // Window Close
    DoMethod(m_Dialog, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    // Open Settings Button
    // Note: MUIC_Text with InputMode behaves like a button but triggers MUIA_Pressed or need to check class specifics.
    // Standard MUIC_Text doesn't usually trigger MUIA_Pressed unless it's a button class subclass or configured.
    // However, with MUIV_InputMode_RelVerify, it acts like a button and usually triggers MUIA_Pressed=FALSE upon release (click).
    DoMethod(m_BtnOpenSettings, MUIM_Notify, MUIA_Pressed, FALSE,
        app, 2, MUIM_Application_ReturnID, APPRETURN_SETTINGS);
}
