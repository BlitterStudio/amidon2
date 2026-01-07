#ifndef WELCOME_DIALOG_H
#define WELCOME_DIALOG_H

#include <libraries/mui.h>

class WelcomeDialog {
public:
    WelcomeDialog();
    ~WelcomeDialog();
    
    Object* Create();
    void InitNotifications(Object* app);
    Object* GetMUIObject() { return m_Dialog; }

private:
    Object* m_Dialog;
    Object* m_BtnOpenSettings;
};

#endif
