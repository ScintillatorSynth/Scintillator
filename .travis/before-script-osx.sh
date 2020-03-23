#!/bin/sh

XCPRETTY='xcpretty -f `xcpretty-travis-formatter`'

# Need to build Vulkan and ffmpeg deps first
cd $TRAVIS_BUILD_DIR/third_party/ffmpeg-ext
mkdir build
cd build
cmake -G Xcode -DCMAKE_OSX_DEPLOYMENT_TARGET=10.10 ..
cmake --build . | $XCPRETTY
cmake --install . | $XCPRETTY

cd $TRAVIS_BUILD_DIR/third_party/vulkan-ext
mkdir build
cd build
cmake -G Xcode -DCMAKE_OSX_DEPLOYMENT_TARGET=10.10 ..
cmake --build . | $XCPRETTY
cmake --install . | $XCPRETTY

cd $TRAVIS_BUILD_DIR/build

cmake -G Xcode -DPYTHON_EXECUTABLE=`which python3` -DCMAKE_OSX_DEPLOYMENT_TARGET=10.10                                 \
    -DSCIN_SCLANG=$HOME/bin/SuperCollider/SuperCollider.app/Contents/MacOS/sclang ..

