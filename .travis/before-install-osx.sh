#!/bin/sh

export HOMEBREW_NO_ANALYTICS=1

# SuperCollider build dependencies
brew -q update
brew -q install libsndfile || brew install libsndfile || exit 1
brew -q install portaudio || exit 2
brew -q install ccache || exit 3
brew -q link qt5 --force || exit 5

# according to https://docs.travis-ci.com/user/caching#ccache-cache
export PATH="/usr/local/opt/ccache/libexec:$PATH"

# To get less noise in xcode output, as some builds are terminated for exceeding maximum log length.
gem install xcpretty

# Checkout and build SuperCollider into $HOME/bin
mkdir $HOME/bin
mkdir $HOME/src
cd src
git clone --recursive https://github.com/supercollider/supercollider
cd supercollider
mkdir build
cd build
cmake -G Xcode -DCMAKE_INSTALL_PREFIX=$HOME/bin -DCMAKE_PREFIX_PATH=`brew --prefix qt5` -DSC_ABLETON_LINK=OFF          \
    -DSC_EL=OFF ..
cmake --build . --target install --config RelWithDebInfo

cd $TRAVIS_BUILD_DIR

# Install Scintillator buildtime dependencies
brew -q unlink python@2
brew -q install ninja doxygen lame libass shtool texi2html nasm

