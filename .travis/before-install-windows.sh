#!/bin/bash

choco install python3 gperf imagemagick.tool
echo "*** installing pyyaml"
cd $HOME
git clone https://github.com/yaml/pyyaml.git pyyaml
cd pyyaml
/c/Python38/python.exe setup.py install