#!/bin/bash

mkdir -p $HOME/artifacts/coverage
cp $TRAVIS_BUILD_DIR/build/src/scinsynth_coverage.json $HOME/artifacts
cp -R $TRAVIS_BUILD_DIR/build/doc $HOME/artifacts
cp -R $TRAVIS_BUILD_DIR/build/report $HOME/artifacts

