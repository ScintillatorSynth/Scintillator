#!/bin/sh

XCPRETTY='xcpretty --simple --no-utf --no-color'

mkdir $TRAVIS_BUILD_DIR/build
cd $TRAVIS_BUILD_DIR/build
cmake --build . --config Release | $XCPRETTY
cmake --install .
cmake --build . --target compare_images --config Release

