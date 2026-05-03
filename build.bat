@echo off
rem Build script for Amidon2 using the sacredbanana Docker image

set IMAGE=sacredbanana/amiga-compiler:m68k-amigaos
set WORK_DIR=%cd%

echo Building Amidon2 using %IMAGE%...

docker run --rm -v "%WORK_DIR%":/work -w /work %IMAGE% /bin/bash -c "rm -rf build && mkdir -p build && cd build && cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/m68k-amigaos.cmake && make VERBOSE=1"

echo Done. Output: build\Amidon2
