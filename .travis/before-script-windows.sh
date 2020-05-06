#!/bin/bash

# cmake -DPYTHON_EXECUTABLE=/c/Python38/python.exe ..
cd $TRAVIS_BUILD_DIR/build
python tools/fetch-binary-deps
cd $TRAVIS_BUILD_DIR/build
cmake -DSCIN_BUILD_DOCS=OFF ..

