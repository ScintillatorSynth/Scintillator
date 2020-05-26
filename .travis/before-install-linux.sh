#!/bin/bash

# Add the SuperCollider PPA
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys FABAEF95
sudo add-apt-repository --yes ppa:supercollider/ppa
sudo apt-get update

# Install all the build and test dependencies
sudo apt-get install --yes      \
    build-essential             \
    cmake                       \
    doxygen                     \
    gperf                       \
    imagemagick                 \
    libjack-jackd2-dev          \
    libwayland-dev              \
    libxcursor-dev              \
    libxi-dev                   \
    libxinerama-dev             \
    libxrandr-dev               \
    pkg-config                  \
    python3-distutils           \
    python3-yaml                \
    supercollider-ide           \
    xvfb                        \
    zlib1g

# Coverage and Linting requires clang-8
if [ $DO_COVERAGE = true ]; then
    sudo apt-get install --yes  \
        clang-8                 \
        clang-format-8          \
        libc++-8-dev            \
        libc++abi-8-dev         \
        libc++abi-8-dev         \
        llvm-8-dev
fi

cd $TRAVIS_BUILD_DIR
python3 tools/fetch-binary-deps.py

