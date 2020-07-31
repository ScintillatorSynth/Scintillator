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
    python3-yaml                \
    supercollider-ide           \
    xvfb                        \
    zlib1g

if [ $DO_COVERAGE = true ]; then
    sudo apt-get install --yes  \
        clang-8                 \
        clang-format-8          \
        libc++-8-dev            \
        libc++abi-8-dev         \
        llvm-8-dev              \
        python3-distutils
else
    sudo add-apt-repository --yes ppa:ubuntu-toolchain-r/test
    sudo apt-get update
    sudo update-alternatives --remove-all gcc
    sudo update-alternatives --remove-all g++
    sudo apt-get install --yes  \
        gcc-7                   \
        g++-7
    sudo update-alternatives --install /usr/bin/gcc gcc `which gcc-7` 10
    sudo update-alternatives --install /usr/bin/g++ g++ `which g++-7` 10
    sudo update-alternatives --set cc /usr/bin/gcc
    sudo update-alternatives --set c++ /usr/bin/g++
fi

cd $TRAVIS_BUILD_DIR
python3 tools/fetch-binary-deps.py

# Crashpad requires SSLv1.1 but Xenial offers 1.0 only. So we build SSLv1.1 from sources as part of the crashpad-ext
# build, and install it locally on this machine, so it will be available at AppDir linking time, to include in the
# Scintillator binary.
sudo cp -R $TRAVIS_BUILD_DIR/build/install-ext/ssl /usr/local/ssl
sudo echo "/usr/local/ssl/lib" > /etc/ld.so.conf.d/openssl-1.1.1g.conf
sudo ldconfig -v || exit 5
