#!/bin/bash

cmake -DSCIN_BUILD_DOCS=ON -DCMAKE_BUILD_TYPE=Coverage -DLLVM_COV="llvm-cov-8" -DLLVM_PROFDATA="llvm-profdata-8"       \
	-DPYTHON_EXECUTABLE=`which python3` -DSCIN_SCLANG=`which sclang` ..

