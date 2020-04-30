#!/bin/bash

if [ $DO_COVERAGE = true ]; then
    mkdir -p $HOME/artifacts/coverage
    cp $TRAVIS_BUILD_DIR/build/src/scinsynth_coverage.json $HOME/artifacts
    cp -R $TRAVIS_BUILD_DIR/build/doc $HOME/artifacts
    cp -R $TRAVIS_BUILD_DIR/build/report $HOME/artifacts
elif [[ -z $TRAVIS_TAG ]]; then
    mkdir -p $HOME/builds
    cp $TRAVIS_BUILD_DIR/bin/scinsynth-x86_64.AppImage $HOME/builds/scinsynth-$TRAVIS_COMMIT-x86_64.AppImage
    shasum -a 256 -b $HOME/builds/scinsynth-$TRAVIS_COMMIT-x86_64.AppImage > $HOME/builds/scinsynth-$TRAVIS_COMMIT-x86_64.sha256
    export S3_URL='http://scintillator-synth-coverage.s3-website-us-west-1.amazonaws.com/builds/scinsynth-'$TRAVIS_COMMIT'-x86_64.AppImage'
    export FWD_HTML='<html><head><meta http-equiv="refresh" content="0; url='$S3_URL'" /></head></html>'
    echo $FWD_HTML > $HOME/builds/scinsynth-latest-x86_64.AppImage.html
else
    mkdir -p $HOME/releases/$TRAVIS_TAG
    cp $TRAVIS_BUILD_DIR/bin/scinsynth-x86_64.AppImage $HOME/releases/$TRAVIS_TAG/.
    cd $HOME/releases/$TRAVIS_TAG
    gzip scinsynth-x86_64.AppImage
    shasum -a 256 -b scinsynth-x86_64.AppImage.gz > scinsynth-x86_64.AppImage.gz.sha256
fi
