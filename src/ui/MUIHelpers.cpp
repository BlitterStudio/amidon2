/*
 * Amidon - a Mastodon client for AmigaOS
 * Copyright (C) 2024 Dimitris Panokostas
 */

#include "MUIHelpers.h"

#include <stdarg.h>
#include <vector>
#include <utility/tagitem.h>
#include <proto/muimaster.h>

extern "C" {

Object* MUIHelpers_NewObject(const char* classname, ...) {
    // printf("DEBUG: MUIHelpers_NewObject for class '%p'\n", classname);
    va_list args;
    va_start(args, classname);
    
    std::vector<struct TagItem> tags; 
    ULONG tag;
    // printf("DEBUG: Parsing tags for %p\n", classname);
    while ((tag = va_arg(args, ULONG)) != TAG_DONE) {
         // printf("DEBUG: Tag: 0x%08lx\n", tag);
        struct TagItem ti;
        ti.ti_Tag = tag;
        ti.ti_Data = va_arg(args, ULONG);
         // printf("DEBUG: Data: 0x%08lx\n", ti.ti_Data);
        tags.push_back(ti);
    }
    struct TagItem endTag = { TAG_DONE, 0 };
    tags.push_back(endTag);
    
    va_end(args);
    
     // printf("DEBUG: Calling MUI_NewObjectA for %p...\n", classname);
    Object* obj = MUI_NewObjectA(classname, tags.data());
     if (!obj) printf("DEBUG: MUI_NewObjectA returned NULL for class '%p'\n", classname);
    return obj;
}

Object* MakeImageButton(const char* imagePath) {
    // A standard MUI image button is typically a Dtpic object (or Bitmap) wrapped in a Group 
    // with a Button Frame and InputMode=RelVerify.
    
    // However, Dtpic inside a group might need careful sizing.
    // Let's try wrapping it in a simple Text object with image escape sequence?
    // No, Dtpic object is cleaner if we want just the image.
    
    // We use a Group with fixed/free dimensions or let the image dictate size.
    // Common pattern:
    // Group (Horizontal=False)
    //    Frame = Button
    //    InputMode = RelVerify
    //    Child = Dtpic ...
    
    return MUIHelpers_NewObject(MUIC_Group,
        MUIA_Frame, MUIV_Frame_Button,
        MUIA_Background, MUII_ButtonBack,
        MUIA_InputMode, MUIV_InputMode_RelVerify,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Child, (IPTR)MUIHelpers_NewObject(MUIC_Dtpic,
            MUIA_Dtpic_Name, (IPTR)imagePath,
            TAG_DONE),
        TAG_DONE);
}

}
