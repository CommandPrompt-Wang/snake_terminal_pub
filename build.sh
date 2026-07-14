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
    echo "Cleaning build and dist..."
    rm -rf build dist
    echo "Done."
    exit 0
fi

# build the project
mkdir -p build
cd build

if [ "$BUILD_RAYLIB" = true ] || [ ! -f CMakeCache.txt ]; then
    cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE" || exit 1
fi

cmake --build . -j"$(nproc)" || exit 1
cd ..

# distribute the build artifacts
rm -rf dist
mkdir -p dist
cp build/snake dist/snake
cp -r resources dist/resources