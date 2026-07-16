#!/bin/bash

# move to the script's directory
cd "$(dirname "$0")"

# parse args
BUILD_TYPE="Release"
BUILD_RAYLIB=false
CLEAN=false

for arg in "$@"; do
    case "$arg" in
        --debug)        BUILD_TYPE="Debug" ;;
        --build-raylib) BUILD_RAYLIB=true  ;;
        --clean)        CLEAN=true         ;;
    esac
done

if [ "$CLEAN" = true ]; then
    echo "Cleaning build-linux and dist-linux..."
    rm -rf build-linux dist-linux
    echo "Done."
    exit 0
fi

# build the project
mkdir -p build-linux
cd build-linux

if [ "$BUILD_RAYLIB" = true ] || [ ! -f CMakeCache.txt ]; then
    cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE" || exit 1
fi

if command -v nproc >/dev/null 2>&1; then
    cmake --build . -j"$(nproc)" || exit 1
else
    cmake --build . -j1 || exit 1
fi
cd ..

# distribute the build artifacts
rm -rf dist-linux
mkdir -p dist-linux
cp build-linux/snake dist-linux/snake
cp -r resources dist-linux/resources
