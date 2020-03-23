#!/bin/sh

XCPRETTY='xcpretty -f `xcpretty-travis-formatter`'

cmake --build . --target swiftshader | $XCPRETTY
cmake --build . | $XCPRETTY
cmake --install . --config Debug | $XCPRETTY

