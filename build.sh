#!/bin/bash

# move to the script's directory
cd "$(dirname "$0")"

# parse args
BUILD_TYPE="Release"
BUILD_RAYLIB=false

for arg in "$@"; do
    case "$arg" in
        --debug)        BUILD_TYPE="Debug" ;;
        --build-raylib) BUILD_RAYLIB=true  ;;
    esac
done

# build the project
mkdir -p build
cd build

if [ "$BUILD_RAYLIB" = true ]; then
    cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE" || exit 1
fi

cmake --build . -j"$(nproc)" || exit 1
cd ..

# distribute the build artifacts
rm -rf dist
mkdir -p dist
cp build/snake dist/snake
cp -r resources dist/resources