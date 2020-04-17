#!/bin/sh

mkdir -p $HOME/builds
tar czf $HOME/builds/scinsynth.app.$TRAVIS_COMMIT.tgz $TRAVIS_BUILD_DIR/bin/scinsynth.app
shashum -a 256 -b $HOME/builds/scinsynth.app.$TRAVIS_COMMIT.tgz > $HOME/builds/scinsynth.app.$TRAVIS_COMMIT.sha256
export S3_URL='http://scintillator-synth-coverage.s3-website-us-west-1.amazonaws.com/builds/scinsynth.app.'$TRAVIS_COMMIT'.tgz'
export FWD_HTML='<html><head><meta http-equiv="refresh" content="0; url='$S3_URL'" /></head></html>'
echo $FWD_HTML > $HOME/builds/scinsynth.app.latest.html

