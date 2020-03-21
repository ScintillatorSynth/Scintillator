#!/bin/bash

# Add the Vulkan SDK PPA
wget -qO - http://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-bionic.list http://packages.lunarg.com/vulkan/lunarg-vulkan-bionic.list
# Add the SuperCollider PPA
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys FABAEF95
sudo add-apt-repository ppa:supercollider/ppa
sudo apt-get update
# Install all the build and test dependencies
sudo apt-get install --yes build-essential clang-8 clang-format-8 cmake doxygen glslang-dev gperf imagemagick          \
	libavcodec-dev libavformat-dev libc++-8-dev libc++abi-8-dev libswscale-dev libvulkan-dev libwayland-dev            \
	libxcursor-dev libxi-dev libxinerama-dev libxrandr-dev llvm-8-dev pkg-config python3-distutils python3-yaml        \
	shaderc spirv-headers spirv-tools supercollider-ide vulkan-headers vulkan-validationlayers xvfb zlib1g

