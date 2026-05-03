#!/bin/bash
# Build script for Amidon2 using the sacredbanana Docker image
# Image includes: NDK 3.2, Roadshow SDK, AmiSSL 5.18, MUI, json-c

IMAGE="sacredbanana/amiga-compiler:m68k-amigaos"
WORK_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "Building Amidon2 using $IMAGE..."

docker run --rm -v "$WORK_DIR":/work -w /work "$IMAGE" /bin/bash -c "
    rm -rf build && \
    mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/m68k-amigaos.cmake && \
    make VERBOSE=1
"

echo "Done. Output: build/Amidon2"
