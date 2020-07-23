#!/bin/sh

XCPRETTY='xcpretty --simple --no-utf --no-color'

cd $TRAVIS_BUILD_DIR/build
cmake --build . --config RelWithDebInfo | $XCPRETTY
cmake --build . --config RelWithDebInfo --target dump_symbols
cmake --install .
cmake --build . --target compare_images --config RelWithDebInfo

