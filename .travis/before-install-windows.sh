#!/bin/bash

choco install python3 gperf imagemagick.tool
echo "*** installing pyyaml"
cd $HOME
git clone https://github.com/yaml/pyyaml.git pyyaml
cd pyyaml
/c/Python38/python.exe setup.py install

echo "*** installing vulkan runtime"
#cd $HOME
#mkdir vulkan
#cd vulkan
#curl https://scintillator-synth-coverage.s3-us-west-1.amazonaws.com/dependencies/windows/vulkan-runtime-components.zip --output vulkan-runtime-components.zip
#unzip vulkan-runtime-components.zip
#cd Vulkan*
#./x86/vulkaninfo.exe

# curl https://scintillator-synth-coverage.s3-us-west-1.amazonaws.com/dependencies/windows/vulkan-runtime.exe --output vulkan-runtime.exe
#./vulkan-runtime.exe /S

echo "*** installing build binary deps"
cd $TRAVIS_BUILD_DIR
/c/Python38/python.exe tools/fetch-binary-deps.py
/c/Python38/python.exe tools/fetch-sclang.py $TRAVIS_HOME/sclang


