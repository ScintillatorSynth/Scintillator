#!/bin/bash

touch $TRAVIS_BUILD_DIR/bin/disable_auto_install
cd $TRAVIS_BUILD_DIR/build
cmake --build . --config Release
cmake --install . --config Release

echo "Configuring host machine to use Swiftshader as a Vulkan device"
powershell 'New-Item -Path "HKLM:\SOFTWARE\Khronos\Vulkan" -Name Drivers'
export SWSHADER_PATH=`cygpath -d $TRAVIS_BUILD_DIR`'\bin\scinsynth-w64\vulkan\icd.d\vk_swiftshader_icd.json'
echo "Setting Swiftshader path to $SWSHADER_PATH"
powershell 'New-ItemProperty -PATH "HKLM:\SOFTWARE\Khronos\Vulkan\Drivers" -Name "'$SWSHADER_PATH'" -Value "0" -PropertyType "DWORD"'

echo "building language config"
cmake --build . --config Release --target sclang_language_config
ps

echo "building test images"
cmake --build . --config Release --target test_images
ps

echo "comparing images"
cmake --build . --config Release --target compare_images
ps
