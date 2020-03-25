#!/bin/sh

cd $TRAVIS_BUILD_DIR/build

cmake -G Xcode -DPYTHON_EXECUTABLE=`which python3` -DCMAKE_OSX_DEPLOYMENT_TARGET=10.10                                 \
    -DSCIN_SCLANG=$HOME/bin/SuperCollider/SuperCollider.app/Contents/MacOS/sclang ..

