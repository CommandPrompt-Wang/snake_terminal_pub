#!/bin/bash

# move to the script's directory
cd "$(dirname "$0")"

# build type: --debug -> Debug, otherwise Release
BUILD_TYPE="Release"
if [ "$1" = "--debug" ]; then
    BUILD_TYPE="Debug"
fi

# build the project
mkdir -p build
cd build && cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE" && make -j$(nproc)

# distribute the build artifacts
cd ..

rm -rf dist
mkdir -p dist
cp build/snake dist/snake
cp -r resources dist/resources