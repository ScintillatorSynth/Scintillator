#!/bin/bash

cd $TRAVIS_BUILD_DIR/build

make install || exit 1

if $DO_COVERAGE; then
    make coverage_report || exit 2
    make sclang_unit_tests || exit 3
    make -j lintall || exit 4
    make -j docs || exit 5
else
    make compare_images || exit 2
fi

