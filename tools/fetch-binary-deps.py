#!/usr/bin/python3

# Script for fetching the binary build-time dependencies for compiling Scintillator. On each supported operating system
# Scintillator relies on several compile-time dependencies. These can be fairly heavy-weight compilations, for example
# on Travis right now the Vulkan SDK compile takes over an hour to perform. In the interest of expediency we have set
# up independent build services for these particular dependencies, and save the build output from Travis to an S3
# instance. This script downloads the os-specific dependent binaries, validates the download, and extracts them to the
# expected location for the rest of the build script to use.

# Run the script from the Scintillator Quark root directory, like this:

# python3 tools/fetch-binary-deps.py

# The script will persist the hash file and the downloaded data, and if re-run it will check if that is the most recent
# build of the binary dependencies, and only download newer dependencies if they exist. To force a redownload and
# re-extraction, simply delete the existing files before running the script.

import os
import platform
import urllib2

def main(argv):
    # Check current directory is the Scintillator root directory.
    if not os.path.exists('Scintillator.quark'):
        print('Please run this script from the Scintillator Quark root directory.')
        sys.exit(1)

    # Make download and extraction directories.
    os.makedirs('build/binary-deps', exist_ok=True)
    os.makedirs('build/install-ext', exist_ok=True)

    if platform.system() == 'Linux':
        os_name = 'linux'
        needed_deps = ['swiftshader-ext']
    elif platform.system() == 'Darwin':
        os_name = 'osx'
        needed_deps = ['ffmpeg-ext', 'vulkan-ext', 'swiftshader-ext']
    else:
        print('Unsupported operating system.')
        sys.exit(1)

    url_base = 'http://scintillator-synth-coverage.s3-website-us-west-1.amazonaws.com/binaries/'


if __name__ == "__main__":
    main(sys.argv[1:])
