#!/bin/bash

touch $TRAVIS_BUILD_DIR/bin/disable_auto_install
cd $TRAVIS_BUILD_DIR/build
cmake --build . --config Release
cmake --install . --config Release
echo "building language config"
cmake --build . --config Release --target $TRAVIS_BUILD_DIR/build/testing/sclang_config.yaml
echo "building test images"
ps
cmake --build . --config Release --target test_images
echo "comparing images"
ps
cmake --build . --config Release --target compare_images




