cmake_minimum_required(VERSION 3.14)
project(amidon VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Local SDK paths (mounted via /amigadev)
set(AMIGADEV /amigadev)
set(MUI_SDK ${AMIGADEV}/MUI50-os3/C)
set(NDK_SDK ${AMIGADEV}/NDK3.2)

# Global Compiler Flags
# -mcrt=clib2: Required for POSIX/C++ stdlib compatibility
# -msoft-float: Required for m68k-amigaos
# -include compat_errno.h: Forces inclusion of necessary error definitions
set(COMMON_FLAGS "-m68020 -msoft-float -O0 -mcrt=clib2 -U_POSIX_SOURCE -include ${CMAKE_SOURCE_DIR}/src/compat_errno.h")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -fno-exceptions -fno-rtti")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mcrt=clib2")

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
    src/HttpClient.cpp
    src/compat.cpp
)

# Executable
add_executable(amidon ${SOURCES})

# Header Search Paths
target_include_directories(amidon PRIVATE
    src
    src/ui
    ${MUI_SDK}/include
    ${NDK_SDK}/Include_H
)

# Library Search Paths
target_link_directories(amidon PRIVATE
    ${MUI_SDK}/lib
)

# Linker options
target_link_options(amidon PRIVATE "LINKER:--allow-multiple-definition")

# Link Libraries
target_link_libraries(amidon PRIVATE
    -lmui
    -latomic
)

# Assets
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
configure_file(${CMAKE_SOURCE_DIR}/icons/GlowIcon/Amidon_OS3.info ${CMAKE_BINARY_DIR}/amidon.info COPYONLY)
