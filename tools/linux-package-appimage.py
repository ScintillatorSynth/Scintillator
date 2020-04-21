#!/usr/bin/python3

# Script used as part of the build process to download the latest version of the linuxdeploy tool (if not present in
# the build already), and invoke the tool to build the appimage binary. It assumes it is run from the Scintillator
# root directory.

import os
import pathlib
import platform
import shutil
import subprocess
import sys
import urllib.request

def main(argv):
    # Call linuxDeploy to build directory from the pieces with --custom-apprun

# LD_LIBRARY_PATH=/home/luken/src/Scintillator/build/install-ext/lib ~/Downloads/linuxdeploy-x86_64.AppImage -d /home/luken/src/Scintillator/build/src/scinsynth.desktop -i /home/luken/src/Scintillator/src/linux/scinsynth.png -e /home/luken/src/Scintillator/build/src/scinsynth --custom-apprun=/home/luken/src/Scintillator/src/linux/AppRun --appdir /home/luken/src/Scintillator/bin/AppDir --output appimage

    return -1

if __name__ == '__main__':
    main(sys.argv[1:])
