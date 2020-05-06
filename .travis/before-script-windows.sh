#!/bin/bash

cd $TRAVIS_BUILD_DIR
/c/Python38/python.exe tools/fetch-binary-deps.py
cd $TRAVIS_BUILD_DIR/build
cmake -DSCIN_BUILD_DOCS=OFF -DPYTHON_EXECUTABLE=/c/Python38/python.exe ..

