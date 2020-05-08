#!/bin/bash

touch $TRAVIS_BUILD_DIR/bin/disable_auto_install
cd $TRAVIS_BUILD_DIR/build
cmake --build . --config Release
cmake --install . --config Release

echo "building language config"
cmake --build . --config Release --target sclang_language_config
ps

echo "building test images"
cmake --build . --config Release --target test_images
ps

echo "comparing images"
cmake --build . --config Release --target compare_images
ps
