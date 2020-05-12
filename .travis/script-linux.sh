#!/bin/bash

cd $TRAVIS_BUILD_DIR/build

make install || exit 1

if [ $DO_COVERAGE = true ]; then
    make coverage_report || exit 2
    make sclang_unit_tests || exit 3
    make lintall || exit 4
    make docs || exit 5
else
    make compare_images || exit 2
fi

