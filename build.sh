# !/bin/bash

# move to the script's directory
cd "$(dirname "$0")"

# build the project
mkdir -p build
cd build && cmake .. && make -j$(nproc)

# distribute the build artifacts
cd ..

rm -rf dist
mkdir -p dist
cp build/snake dist/snake
cp -r resources dist/resources