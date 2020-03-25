#!/bin/sh

XCPRETTY='xcpretty --simple --no-utf --no-color'

cmake --build . --config Release | $XCPRETTY
cmake --install .

