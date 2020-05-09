#!/bin/bash

choco install python3 gperf imagemagick.tool
echo "*** installing pyyaml"
cd $HOME
git clone https://github.com/yaml/pyyaml.git pyyaml
cd pyyaml
/c/Python38/python.exe setup.py install

echo "*** installing build binary deps"
cd $TRAVIS_BUILD_DIR
/c/Python38/python.exe tools/fetch-binary-deps.py
/c/Python38/python.exe tools/fetch-sclang.py $TRAVIS_HOME/sclang


