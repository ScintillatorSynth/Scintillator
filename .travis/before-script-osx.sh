#!/bin/sh

touch $TRAVIS_BUILD_DIR/bin/disable_auto_install
cd $TRAVIS_BUILD_DIR/build

cmake -G Xcode
    -DSCIN_USE_CRASHPAD=OFF                                                                                            \
    -DPYTHON_EXECUTABLE=`which python3`                                                                                \
    -DSCIN_BUILD_DOCS=OFF                                                                                              \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13                                                                                \
    -DSCIN_SCLANG=$TRAVIS_HOME/sclang/SuperCollider/SuperCollider.app/Contents/MacOS/sclang                            \
    ..

