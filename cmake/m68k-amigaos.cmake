# CMake toolchain file for m68k-amigaos
set(CMAKE_SYSTEM_NAME AmigaOS)
set(CMAKE_SYSTEM_VERSION 3)
set(CMAKE_SYSTEM_PROCESSOR m68k)

# Specify the cross-compiler
set(TC_PATH /opt/m68k-amigaos)
set(CMAKE_C_COMPILER ${TC_PATH}/bin/m68k-amigaos-gcc)
set(CMAKE_CXX_COMPILER ${TC_PATH}/bin/m68k-amigaos-g++)
set(CMAKE_ASM_COMPILER ${TC_PATH}/bin/m68k-amigaos-as)
set(CMAKE_AR ${TC_PATH}/bin/m68k-amigaos-ar)
set(CMAKE_RANLIB ${TC_PATH}/bin/m68k-amigaos-ranlib)

# Search paths
set(CMAKE_FIND_ROOT_PATH ${TC_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
