#!/bin/bash

cd $TRAVIS_BUILD_DIR/build
cmake -DSCIN_BUILD_DOCS=OFF -DSCIN_SCLANG=$TRAVIS_HOME/sclang/SuperCollider/sclang.exe -DPYTHON_EXECUTABLE=/c/Python38/python.exe -DCMAKE_GENERATOR_PLATFORM=x64 ..
