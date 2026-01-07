/*
 * Amidon - a Mastodon client for AmigaOS
 * Copyright (C) 2024 Dimitris Panokostas
 */

#ifndef UI_MUIHELPERS_H
#define UI_MUIHELPERS_H

#include <libraries/mui.h>

#ifndef IPTR
typedef unsigned long IPTR;
#endif

// Define our own wrapper for MUI_NewObject to handle varargs
#ifdef MUI_NewObject
#undef MUI_NewObject
#endif

extern "C" {
    // Wrapper to handle varargs for MUI object creation
    Object* MUIHelpers_NewObject(const char* className, ...);

// Helper to create simple image buttons
Object* MakeImageButton(const char* imagePath);
    ULONG DoMethod(Object *obj, ULONG methodID, ...);
}

#endif // UI_MUIHELPERS_H
