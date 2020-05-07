#!/bin/bash

touch $TRAVIS_BUILD_DIR/bin/disable_auto_install
cd $TRAVIS_BUILD_DIR/build
cmake --build . --config Release
cmake --install . --config Release
cmake --build . --config Release --target compare_images
