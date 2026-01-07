cmake_minimum_required(VERSION 3.14) # Bumped for FetchContent
project(amidon VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcrt=clib2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcrt=clib2")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mcrt=clib2")

# Ensure feature checks can find libraries
set(CMAKE_REQUIRED_LIBRARIES "-lnet -lauto")

include(FetchContent)

# JSON Library
# JSON Library
# FetchContent_Declare(
#     json
#     URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
# )
# FetchContent_MakeAvailable(json)

set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "Build zlib examples" FORCE)
FetchContent_Declare(
    zlib
    URL https://zlib.net/zlib-1.3.1.tar.gz
)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(zlib)

# Source files
set(SOURCES
    src/main.cpp
    # src/App.cpp
    # src/ui/MainWindow.cpp
    # src/ui/MUIHelpers.cpp
    # src/ui/WelcomeDialog.cpp
    # src/ui/SettingsDialog.cpp
    # src/ui/LoginDialog.cpp
    # src/MastodonAPI.cpp
    # src/HttpClient.cpp
    src/compat.cpp
    src/NetworkTest.cpp
    src/OsSocket.c
)

# Includes
include_directories(
    src
    src/ui
    ${zlib_SOURCE_DIR}
    ${zlib_BINARY_DIR}
)

# Executable
add_executable(amidon ${SOURCES})

# System libs (clib2)
# These lines are already present above, no change needed# System libs
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m68020 -msoft-float -O0 -U_POSIX_SOURCE -include ${CMAKE_SOURCE_DIR}/src/compat_errno.h")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m68020 -msoft-float -O0 -U_POSIX_SOURCE -include ${CMAKE_SOURCE_DIR}/src/compat_errno.h -fno-exceptions -fno-rtti")
 set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
 
 # force allow multiple definition
 target_link_options(amidon PRIVATE "LINKER:--allow-multiple-definition")

# Link Libraries
target_link_libraries(amidon PRIVATE
    # nlohmann_json::nlohmann_json
    zlibstatic
    -latomic
    -lnet
    -lunix
)

# Copy assets to build directory
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})

# Copy default icon
configure_file(${CMAKE_SOURCE_DIR}/icons/GlowIcon/Amidon_OS3.info ${CMAKE_BINARY_DIR}/amidon.info COPYONLY)
