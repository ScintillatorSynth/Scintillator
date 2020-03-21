#!/bin/sh

cmake --build . --target swiftshader
cmake --build .
cmake --install . --config Debug

