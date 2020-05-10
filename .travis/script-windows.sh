#!/bin/bash

cd $TRAVIS_BUILD_DIR/build
cmake --build . --config Release
cmake --install . --config Release

echo "Configuring host machine to use Swiftshader as a Vulkan device"
powershell 'New-Item -Path "HKLM:\SOFTWARE\Khronos\Vulkan" -Name Drivers'
export SWSHADER_PATH=`cygpath -w $TRAVIS_BUILD_DIR`'\bin\scinsynth-w64\vulkan\icd.d\vk_swiftshader_icd.json'
echo "Setting Swiftshader path to $SWSHADER_PATH"
powershell 'New-ItemProperty -PATH "HKLM:\SOFTWARE\Khronos\Vulkan\Drivers" -Name "'$SWSHADER_PATH'" -Value "0" -PropertyType "DWORD"'

cmake --build . --config Release --target compare_images

