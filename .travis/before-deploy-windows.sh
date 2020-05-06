#!/bin/bash

if [[ -z $TRAVIS_TAG ]]; then
    mkdir -p $HOME/builds/scinsynth-w64
    powershell "compress-archive $TRAVIS_BUILD_DIR/bin/scinsynth-w64 $HOME/builds/scinsynth-w64-$TRAVIS_COMMIT.zip"
    certutil -hashfile $HOME/builds/scinsynth-w64-$TRAVIS_COMMIT.zip SHA256 | sed -n '2,2p;2q' > $HOME/builds/scinsynth-w64-$TRAVIS_COMMIT.zip.sha256
    export S3_URL='http://scintillator-synth-coverage.s3-website-us-west-1.amazonaws.com/builds/scinsynth-w64-'$TRAVIS_COMMIT'.zip'
    export FWD_HTML='<html><head><meta http-equiv="refresh" content="0; url='$S3_URL'" /></head></html>'
    echo $FWD_HTML > $HOME/builds/scinsynth-latest-x86_64.AppImage.html
else
    mkdir -p $HOME/releases/$TRAVIS_TAG
    powershell "compress-archive $TRAVIS_BUILD_DIR/bin/scinsynth-w64 $HOME/releases/$TRAVIS_TAG/scinsynth-w64.zip"
    certutil -hashfile $HOME/releases/$TRAVIS_TAG/scinsynth-w64.zip SHA256 | sed -n '2,2p;2q' > $HOME/releases/$TRAVIS_TAG/scinsynth-w64.zip.sha256
fi
