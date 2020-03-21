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

cd $TRAVIS_BUILD_DIR/build

cmake -G Xcode -DPYTHON_EXECUTABLE=`which python3`                                                                     \
    -DSCIN_SCLANG=$HOME/bin/SuperCollider/SuperCollider.app/Contents/MacOS/sclang ..

