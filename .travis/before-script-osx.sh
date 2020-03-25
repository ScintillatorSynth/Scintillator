#!/bin/sh

cd $TRAVIS_BUILD_DIR/build

cmake -G Xcode -DPYTHON_EXECUTABLE=`which python3` -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15                                 \
    -DSCIN_SCLANG=/Applications/SuperCollider.app/Contents/MacOS/sclang ..

