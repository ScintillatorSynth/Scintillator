#!/bin/bash

touch $TRAVIS_BUILD_DIR/bin/disable_auto_install
cd $TRAVIS_BUILD_DIR/build

if [ $DO_COVERAGE = true ]; then
    cmake -DSCIN_BUILD_DOCS=ON -DCMAKE_BUILD_TYPE=Coverage -DLLVM_COV="llvm-cov-8" -DLLVM_PROFDATA="llvm-profdata-8"   \
        -DPYTHON_EXECUTABLE=`which python3` -DSCIN_SCLANG=`which sclang` ..
else
    PATH=/usr/local/bin:$PATH cmake                                                                                    \
        -DSCIN_BUILD_DOCS=OFF                                                                                          \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo                                                                              \
        -DPYTHON_EXECUTABLE=`which python3`                                                                            \
        -DSCIN_SCLANG=`which sclang`                                                                                   \
        ..
fi

