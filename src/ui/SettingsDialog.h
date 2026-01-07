#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <libraries/mui.h>
#include <string>

class SettingsDialog {
public:
    SettingsDialog();
    ~SettingsDialog();
    
    Object* Create();
    void InitNotifications(Object* app);
    Object* GetMUIObject() { return m_Dialog; }
    
    // Getters for settings values
    std::string GetInstanceURL();

private:
    Object* m_Dialog;
    Object* m_StrInstance;
    Object* m_BtnLogin;
    Object* m_BtnSave;
    Object* m_BtnCancel;
};

#endif
