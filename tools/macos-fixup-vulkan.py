#!/usr/bin/python3

import os
import sys

def main(argv):
    if len(argv) != 1:
        print('macos-fixup-vulkan.py <scinsynth.app path>')
        sys.exit(2)
    app_path = argv[0];
    link_name = app_path + "/Contents/MacOS/libvulkan.1.dylib"
    if os.path.lexists(link_name):
        os.remove(link_name)
    lib_name = os.readlink(app_path + "/Contents/Frameworks/libvulkan.1.dylib")
    os.symlink("../Frameworks/" + lib_name, link_name)

if __name__ == "__main__":
    main(sys.argv[1:])

