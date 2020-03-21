#!/bin/sh

cmake -G Xcode -DPYTHON_EXECUTABLE=`which python3`                                                                     \
    -DSCIN_SCLANG=$HOME/bin/SuperCollider/SuperCollider.app/Contents/MacOS/sclang ..

