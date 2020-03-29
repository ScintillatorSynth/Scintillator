#!/bin/bash

cd $TRAVIS_BUILD_DIR/build

make -j2 swiftshader-local-install
make -j2 install || exit 1
make coverage_report || exit 2
make sclang_unit_tests || exit 3
make -j2 lintall || exit 4
make -j2 docs || exit 5

