#!/bin/sh

cd $TRAVIS_BUILD_DIR/build

cmake -G Xcode -DPYTHON_EXECUTABLE=`which python3` ..

