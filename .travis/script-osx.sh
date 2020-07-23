#!/bin/sh

XCPRETTY='xcpretty --simple --no-utf --no-color'

cd $TRAVIS_BUILD_DIR/build
cmake --build . --config RelWithDebInfo | $XCPRETTY || exit 1
cmake --build . --config RelWithDebInfo --target dump_symbols || exit 2
cmake --install . --config RelWithDebInfo || exit 3
cmake --build . --target compare_images --config RelWithDebInfo || exit 4

