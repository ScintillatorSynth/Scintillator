#!/bin/sh

# Need to build Vulkan and ffmpeg deps first
cd $TRAVIS_BUILD_DIR/third_party/ffmpeg-ext
mkdir build
cd build
cmake -G Xcode ..
cmake --build .
cmake --install .

cd $TRAVIS_BUILD_DIR/third_party/vulkan-ext
mkdir build
cd build
cmake -G Xcode ..
cmake --build .
cmake --install .

cmake --build . --target swiftshader
cmake --build .
cmake --install . --config Debug

