#!/bin/bash

cd $TRAVIS_BUILD_DIR/build

make install || exit 1
make dump_symbols || exit 2

if [ $DO_COVERAGE = true ]; then
    make coverage_report || make log_crashes || exit 3
    make sclang_unit_tests || make log_crashes || exit 4
    make lintall || exit 5
    make docs || exit 6
else
    make compare_images || make log_crashes || exit 3
fi

