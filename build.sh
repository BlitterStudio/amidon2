#!/bin/bash
# Build script for Amidon using the amigadev/crosstools container

IMAGE="amigadev/crosstools:m68k-amigaos"
WORK_DIR=$(pwd)

echo "Building Amidon using $IMAGE..."

docker run --rm -v "$WORK_DIR":/work "$IMAGE" /bin/bash -c "
    mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/m68k-amigaos.cmake && \
    make
"
