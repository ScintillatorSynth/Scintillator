#!/bin/bash

cd $TRAVIS_BUILD_DIR/build

make -j install || exit 1
make coverage_report || exit 2
make sclang_unit_tests || exit 3
make -j lintall || exit 4
make -j docs || exit 5

