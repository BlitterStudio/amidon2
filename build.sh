#!/bin/bash
# Build script for Amidon using the amigadev/crosstools container

IMAGE="amigadev/crosstools:m68k-amigaos"
WORK_DIR=$(pwd)

# Find AmigaDev directory (assuming it's in the same parent as the project)
AMIGADEV_PATH="$(cd "$(dirname "$0")/../../AmigaDev" && pwd)"

echo "Building Amidon using $IMAGE..."
echo "Using local SDKs from $AMIGADEV_PATH"

docker run --rm -v "$WORK_DIR":/work -v "$AMIGADEV_PATH":/amigadev "$IMAGE" /bin/bash -c "
    mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/m68k-amigaos.cmake && \
    make
"

