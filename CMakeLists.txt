cmake_minimum_required(VERSION 3.14)
project(Amidon2 VERSION 0.2 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# clib2 runtime
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -mcrt=clib2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcrt=clib2")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mcrt=clib2")

# Target CPU and optimisation
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -m68020 -msoft-float -O2 -Wall -Wno-multichar")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m68020 -msoft-float -O2 -Wall -Wno-multichar -fno-exceptions -fno-rtti")

# AmiSSL for HTTPS
add_definitions(-DHAVE_AMISSL)

# Prevent POSIX_SOURCE from hiding AmigaOS definitions
add_definitions(-U_POSIX_SOURCE)

# Include paths
include_directories(
    src
    src/logic
    src/ui
    src/vendor
    vendor/json-c
    vendor/json-c/include/json-c
)

# Vendored json-c source (rebuilt from source to avoid base-relative reloc issues)
set(JSON_C_SOURCES
    vendor/json-c/arraylist.c
    vendor/json-c/debug.c
    vendor/json-c/json_c_version.c
    vendor/json-c/json_object.c
    vendor/json-c/json_object_iterator.c
    vendor/json-c/json_patch.c
    vendor/json-c/json_pointer.c
    vendor/json-c/json_tokener.c
    vendor/json-c/json_util.c
    vendor/json-c/json_visit.c
    vendor/json-c/linkhash.c
    vendor/json-c/printbuf.c
    vendor/json-c/random_seed.c
    vendor/json-c/strerror_override.c
)

# Source files
set(SOURCES
    src/main.cpp
    src/App.cpp
    src/ui/MainWindow.cpp
    src/ui/MUIHelpers.cpp
    src/ui/WelcomeDialog.cpp
    src/ui/SettingsDialog.cpp
    src/ui/LoginDialog.cpp
    src/MastodonAPI.cpp
    src/CacheManager.cpp
    src/AsyncHttpService.cpp
    src/HttpRequest.cpp
    # Logic (platform-free)
    src/logic/TimelineParser.cpp
    src/logic/AccountParser.cpp
    src/logic/SettingsParser.cpp
    src/logic/TootFormatter.cpp
    src/logic/UrlBuilder.cpp
    src/logic/CredsParser.cpp
    # HTMLview network hook (C, compiled with HAVE_AMISSL)
    src/vendor/htmlview_nethook.c
)

add_executable(Amidon2 ${SOURCES} ${JSON_C_SOURCES})

# Link libraries
target_link_libraries(Amidon2 PRIVATE
    -lmui
    -lamisslauto
    -latomic
)

# Allow multiple definitions (clib2 / amisslauto interplay)
target_link_options(Amidon2 PRIVATE "LINKER:--allow-multiple-definition")

# Copy assets to build directory
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})

# Copy default icon
configure_file(
    ${CMAKE_SOURCE_DIR}/icons/GlowIcon/Amidon_OS3.info
    ${CMAKE_BINARY_DIR}/Amidon2.info
    COPYONLY
)
