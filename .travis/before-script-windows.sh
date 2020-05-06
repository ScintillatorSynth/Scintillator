#!/bin/bash

# cmake -DPYTHON_EXECUTABLE=/c/Python38/python.exe ..
mkdir $TRAVIS_BUILD_DIR/build
cd $TRAVIS_BUILD_DIR/build
cmake ..

