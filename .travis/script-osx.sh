#!/bin/sh

XCPRETTY='xcpretty --simple --no-utf --no-color'

pwd

cmake --build . --config Release | $XCPRETTY
cmake --install .

