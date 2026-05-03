# AGENTS.md - Amidon2

Amidon2 is a Mastodon client for classic AmigaOS 3.x, targeting Motorola 68020+ (m68k).
Built with C++14, MUI (Magic User Interface) for GUI, and cross-compiled via Docker.

## Build Commands

All builds run inside a Docker container (`sacredbanana/amiga-compiler:m68k-amigaos`).
The host must have Docker running.

```bash
# Full build (Windows CMD)
build.bat

# Full build (PowerShell)
.\build.ps1

# Full build (Linux/macOS)
./build.sh

# Manual build inside Docker container
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/m68k-amigaos.cmake
make

# Verbose build (inside container)
make VERBOSE=1

# Clean rebuild (inside container)
cd build && make clean && make
```

Output: `build/Amidon2` (m68k AmigaOS executable, ~330KB).

## Testing

No test framework is configured. No unit tests exist.
Manual testing is done by running the binary on AmigaOS (or WinUAE).

## Linting / Formatting

No clang-format, clang-tidy, or cppcheck configuration exists.
Follow the code style conventions below and match existing files.

## Project Structure

```
src/
  main.cpp              Entry point
  App.cpp/h             Application lifecycle, MUI app object, event loop
  MastodonAPI.cpp/h     Mastodon REST API client (auth, timelines, posting)
  HttpClient.cpp/h      HTTP via bsdsocket.library + AmiSSL (no curl dependency)
  CacheManager.cpp/h    Persistent cache (credentials, tokens, avatars)
  Constants.h           App return ID macros
  ui/
    MainWindow.cpp/h    Main window with nav list, page group, timeline, compose
    MUIHelpers.cpp/h    Varargs wrapper for MUI_NewObject, helper utilities
    WelcomeDialog.cpp/h First-run welcome dialog
    SettingsDialog.cpp/h Instance URL configuration
    LoginDialog.cpp/h   OAuth authorization code entry
  vendor/
    HTMLview_mcc.h      Vendored HTMLview.mcc header (from htmlview-midwan repo)
    htmlview_nethook.c/h Reference load hook for HTMLview (bsdsocket + AmiSSL)
cmake/
  m68k-amigaos.cmake   Cross-compiler toolchain file
assets/                 PNG images used by the UI
icons/                  AmigaOS icon sets (GlowIcon, MagicWB, etc.)
```

## Critical Compiler Constraints

These flags are set in CMakeLists.txt and fundamentally shape how code must be written:

- **`-fno-exceptions`** — C++ exceptions are disabled. Never use `throw`, `try`, or `catch`.
- **`-fno-rtti`** — RTTI is disabled. Never use `dynamic_cast` or `typeid`.
- **`-mcrt=clib2`** — Uses clib2 runtime, not newlib. Some POSIX functions are missing or stubbed.
- **`-m68020 -msoft-float`** — Targets 68020 CPU with software floating point. Avoid float-heavy code.
- **`-DHAVE_AMISSL`** — AmiSSL support is compiled in for HTTPS.
- **`-std=c++14`** — Do not use C++17 or later features.

## Dependencies

All dependencies are provided by the Docker image or vendored in-tree:

| Dependency | Source | Purpose |
|---|---|---|
| MUI 5 (muimaster.library) | Docker SDK | GUI toolkit |
| bsdsocket.library | AmigaOS ROM | TCP/IP networking |
| AmiSSL (amisslmaster.library) | Docker SDK | TLS/HTTPS via OpenSSL |
| json-c | Docker SDK (`libjson-c.a`) | JSON parsing and building |
| TextEditor.mcc | Docker SDK | Multiline text editor gadget |
| HTMLview.mcc | User must install | HTML content rendering |
| libatomic | Docker SDK | Atomic operations for C++ |

Linker flags: `-lmui -lamisslauto -ljson-c -latomic`

## Code Style

### Naming Conventions

| Element             | Convention         | Example                              |
|---------------------|--------------------|--------------------------------------|
| Classes             | PascalCase         | `MainWindow`, `CacheManager`         |
| Structs             | PascalCase         | `Status`, `AppRegistration`          |
| Methods (all)       | PascalCase         | `FetchTimeline()`, `HasSettings()`   |
| Member variables    | `m_` + PascalCase  | `m_App`, `m_MainWindow`, `m_Running` |
| Local variables     | camelCase          | `instance`, `fsize`, `readSize`      |
| Parameters          | camelCase          | `const std::string& instance`        |
| Macros / Constants  | UPPER_SNAKE_CASE   | `APPRETURN_LOGIN`, `MAKE_ID`         |
| Type aliases        | PascalCase         | `HttpCallback`                       |

### Headers

- Use `#ifndef` / `#define` guards, not `#pragma once`:
  ```cpp
  #ifndef HTTP_CLIENT_H
  #define HTTP_CLIENT_H
  // ...
  #endif // HTTP_CLIENT_H
  ```
- Guard names: `FILENAME_H` or `DIRECTORY_FILENAME_H` (e.g. `UI_MAINWINDOW_H`).

### Includes — Order

1. Corresponding header (`#include "App.h"`)
2. Other local project headers (quotes)
3. `extern "C" { }` block for AmigaOS C headers
4. Standard library headers (angle brackets)

```cpp
#include "App.h"
#include "ui/MainWindow.h"
#include "CacheManager.h"

extern "C" {
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
}

#include <cstdio>
#include <vector>
```

AmigaOS SDK headers (`proto/`, `clib/`, `libraries/`) **must** be wrapped in `extern "C" { }` inside `.cpp` files.

### Formatting

- **Indentation**: 4 spaces (no tabs)
- **Braces**: K&R style — opening brace on same line
- **Line length**: soft limit ~120 characters
- Spaces after keywords (`if (`, `while (`, `for (`), around operators, no space before `(` in calls

```cpp
void App::InitializeMUI() {
    if (!SocketBase) {
        SocketBase = OpenLibrary("bsdsocket.library", 3);
        if (!SocketBase) {
            printf("FATAL: Failed to open bsdsocket.library!\n");
        }
    }
}
```

### Types

- Use `std::string` for strings, `const char*` only for C API interop and literals.
- Use `std::unique_ptr` with `std::make_unique` for owned heap objects.
- Use `int`, `long`, `size_t` for general integers; use `ULONG`, `LONG`, `IPTR` for AmigaOS API types.
- Use `Object*` (raw pointer) for MUI objects — MUI manages their lifecycle via `MUI_DisposeObject`.
- Prefer references (`const std::string&`) for required parameters, pointers for nullable/optional.
- `struct Library*` for AmigaOS library bases (e.g. `UtilityBase`, `SocketBase`).

### Error Handling

No exceptions. Use these patterns instead:

1. **Callbacks**: `std::function<void(bool success, const T& result)>` for async operations
2. **Bool returns**: `bool SaveToken(...)` for simple success/failure
3. **Null checks with printf**: check pointers before use, log with `printf` or `fprintf(stderr, ...)`
4. **Always guard callbacks**: `if (callback) callback(false, {});`

```cpp
m_Client.PostWithHeaders(url, payload, authHeader, contentType,
    [callback](const std::string& response, int code) {
        if (code == 200) {
            if (callback) callback(true, result);
        } else {
            fprintf(stderr, "Request failed with code: %d\n", code);
            if (callback) callback(false, {});
        }
    });
```

### JSON

Use **json-c** (linked as `libjson-c.a`). Always check return values and validate types before accessing.

```cpp
// Parsing
json_object* root = json_tokener_parse(jsonStr);
if (!root) { /* handle error */ }
json_object* val = NULL;
if (json_object_object_get_ex(root, "key", &val) &&
    json_object_is_type(val, json_type_string)) {
    std::string str = json_object_get_string(val);
}
json_object_put(root);

// Building
json_object* obj = json_object_new_object();
json_object_object_add(obj, "key", json_object_new_string(someString.c_str()));
const char* json = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY);
json_object_put(obj);
```

### HTTP Client

`HttpClient` uses bsdsocket.library directly with AmiSSL for TLS. No curl dependency.
Public API: `Get()`, `Post()`, `GetWithHeaders()`, `PostWithHeaders()` — all async via callbacks.
AmiSSL bases are opened/closed per-request inside `PerformRequest()`.

### File I/O

Use C-style `FILE*` operations (`fopen`, `fread`, `fwrite`, `fclose`), not C++ streams.
Always check the file pointer before use and close after.

### MUI Object Creation

Use `MUIHelpers_NewObject()` (varargs wrapper) instead of `MUI_NewObject` directly.
Always terminate tag lists with `TAG_DONE`. Cast string literals to `(IPTR)`.

```cpp
m_Window = MUIHelpers_NewObject(MUIC_Window,
    MUIA_Window_Title, (IPTR)"My Window",
    MUIA_Window_RootObject, (IPTR)rootGroup,
    TAG_DONE);
```

### HTMLview.mcc Integration

HTMLview is used for rendering toot HTML content. The vendored nethook (`src/vendor/htmlview_nethook.c`) handles
loading URLs via bsdsocket + AmiSSL (http/https), local files (PROGDIR:, file://).

```cpp
// Initialize nethook (once, in constructor)
HTMLviewNet_InitHook(&m_NetHook);

// Create HTMLview with scrollbars and hooks
Object* htmlScrollgroup = MUIHelpers_NewObject(MUIC_HTMLview,
    MUIA_HTMLview_Scrollbars, MUIV_HTMLview_Scrollbars_Auto,
    MUIA_HTMLview_LoadHook, (IPTR)&m_NetHook,
    MUIA_HTMLview_ImageLoadHook, (IPTR)&m_NetHook,
    MUIA_HTMLview_Contents, (IPTR)"",
    TAG_DONE);

// Extract the actual HTMLview from the Scrollgroup wrapper
ULONG ptr = 0;
GetAttr(MUIA_ScrollGroup_HTMLview, htmlScrollgroup, &ptr);
m_Htmlview = (Object*)ptr;

// Set HTML content
SetAttrs(m_Htmlview, MUIA_HTMLview_Contents, (IPTR)htmlString, TAG_DONE);
```

Use `static std::string` to keep HTML strings alive for MUI when setting `MUIA_HTMLview_Contents`.

### Comments

- Use `//` for single-line comments.
- Use `/* */` block comments only for file-level copyright headers.
- No Doxygen. Keep comments brief and practical.

### Namespaces

- Classes live in the global namespace.
- Use anonymous namespaces in `.cpp` files for file-local helpers.

```cpp
namespace {
    bool FileExists(const std::string& path) { ... }
}
```

### UI Dialog Pattern

All dialogs follow the same structure:
1. Class with `Create()` returning `Object*`, `InitNotifications(Object* app)`, `GetMUIObject()`
2. Add to app via `DoMethod(m_App, OM_ADDMEMBER, (IPTR)dialog->Create())`
3. Show/hide via `SetAttrs(obj, MUIA_Window_Open, TRUE/FALSE, TAG_DONE)`
4. Wire actions via `MUIM_Application_ReturnID` constants defined in `Constants.h`

### Known Build Warnings (Harmless)

- json-c linker warnings about base-relative relocations — cosmetic, do not affect runtime
- `%lu` format warnings on m68k where `unsigned long` is 32-bit — pre-existing in json-c and nethook code
- Nethook signedness warnings for `CONST_STRPTR` vs `char*` — AmigaOS SDK type mismatch
