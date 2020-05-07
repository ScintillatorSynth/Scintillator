#!/bin/bash

cd $TRAVIS_BUILD_DIR
/c/Python38/python.exe tools/fetch-binary-deps.py
cd $TRAVIS_BUILD_DIR/build
cmake -DSCIN_BUILD_DOCS=OFF -DSCIN_SCLANG=/c/Program\ Files/SuperCollider-3.9.3/sclang.exe -DPYTHON_EXECUTABLE=/c/Python38/python.exe -DCMAKE_GENERATOR_PLATFORM=x64 ..

