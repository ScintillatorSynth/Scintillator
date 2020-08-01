#!/bin/bash

cd $TRAVIS_BUILD_DIR/build

make install || exit 1
make dump_symbols || exit 2

if [ $DO_COVERAGE = true ]; then
    make coverage_report || exit 3
    make sclang_unit_tests || exit 4
    make lintall || exit 5
    make docs || exit 6
else
    # The SuperCollider PPA provides SuperCollider 3.6.6 for Xenial, which is too old to support unit tests. We delete
    # the unit tests from the quark so that the library can compile. The unit tests are run on the coverage build, which
    # is on Bionic and has a newer version of SC. We still want to "smoke test" the built binary, so we run the image
    # comparison tests only.
    rm -rf $TRAVIS_BUILD_DIR/tests
    make compare_images || exit 3
fi

# extra catch in the bottom to break build if scinsynth crashed *at any time*
make log_crashes || exit 7

