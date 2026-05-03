#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

mkdir -p "$PROJECT_DIR/build-tests"
cd "$PROJECT_DIR/build-tests"
cmake "$PROJECT_DIR/tests"
make
./amidon2_tests
