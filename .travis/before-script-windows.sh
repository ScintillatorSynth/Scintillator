#!/bin/bash

# cmake -DPYTHON_EXECUTABLE=/c/Python38/python.exe ..
cd $TRAVIS_BUILD_DIR
python tools/fetch-binary-deps.py
cd $TRAVIS_BUILD_DIR/build
cmake -DSCIN_BUILD_DOCS=OFF ..

