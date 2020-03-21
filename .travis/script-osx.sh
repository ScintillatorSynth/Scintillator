#!/bin/sh

XCPRETTY='xcpretty --simple --no-utf --no-color'

cmake --build . --target swiftshader | $XCPRETTY
cmake --build . | $XCPRETTY
cmake --install . --config Debug | $XCPRETTY

