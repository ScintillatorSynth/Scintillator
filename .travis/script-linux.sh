#!/bin/bash

make -j2 swiftshader-local
make -j2 install
make coverage_report
make sclang_unit_tests
make -j2 lintall
make -j2 docs

