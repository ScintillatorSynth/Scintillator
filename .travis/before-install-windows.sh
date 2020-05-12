#!/bin/bash

choco install python3 gperf imagemagick.tool
echo "*** installing pyyaml"
cd $HOME
git clone https://github.com/yaml/pyyaml.git pyyaml
cd pyyaml
/c/Python38/python.exe setup.py install

echo "*** installing Vulkan SDK"
cd $HOME
curl https://scintillator-synth-coverage.s3-us-west-1.amazonaws.com/dependencies/windows/VulkanSDK-1.2.135.0-Installer.exe --output vulkan-sdk.exe
powershell "./vulkan-sdk.exe /S"

cd $TRAVIS_BUILD_DIR
/c/Python38/python.exe tools/fetch-sclang.py $TRAVIS_HOME/sclang
