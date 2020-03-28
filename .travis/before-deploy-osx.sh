#!/bin/sh

mkdir -p $HOME/artifacts
tar czf $HOME/artifacts/scinsynth.app.tgz $TRAVIS_BUILD_DIR/bin/scinsynth.app

