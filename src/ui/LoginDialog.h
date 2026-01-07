#ifndef LOGIN_DIALOG_H
#define LOGIN_DIALOG_H

#include <libraries/mui.h>
#include <string>

class LoginDialog {
public:
    LoginDialog();
    ~LoginDialog();
    
    Object* Create();
    void InitNotifications(Object* app);
    Object* GetMUIObject() { return m_Dialog; }

    void SetAuthURL(const char* url);
    std::string GetAuthCode();
    
private:
    Object* m_Dialog;
    Object* m_StrAuthURL;
    Object* m_StrCode;
    Object* m_BtnLogin;
    Object* m_BtnCancel;
};

#endif
