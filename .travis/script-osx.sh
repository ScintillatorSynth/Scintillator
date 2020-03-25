#!/bin/sh

XCPRETTY='xcpretty --simple --no-utf --no-color'

cd $TRAVIS_BUILD_DIR/build
cmake --build . --config Release | $XCPRETTY
cmake --install .

