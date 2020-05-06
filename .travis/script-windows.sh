#!/bin/bash

cd $TRAVIS_BUILD_DIR/build
cmake --build . --config Release
