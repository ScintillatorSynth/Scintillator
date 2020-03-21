#!/bin/sh

export HOMEBREW_NO_ANALYTICS=1

# SuperCollider build dependencies
brew update
brew install libsndfile || brew install libsndfile || exit 1
brew install portaudio || exit 2
brew install ccache || exit 3
brew link qt5 --force || exit 5

# according to https://docs.travis-ci.com/user/caching#ccache-cache
export PATH="/usr/local/opt/ccache/libexec:$PATH"

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
brew install ninja doxygen lame libass shtool texi2html nasm

