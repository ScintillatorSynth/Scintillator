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
# build of the binary dependencies, and only download newer dependencies if they exist. To force a re-download and
# re-extraction, simply delete the existing files before running the script.

import os
import pathlib
import platform
import shutil
import sys
import urllib.request

def main(argv):
    # Check current directory is the Scintillator root directory.
    if not os.path.exists('Scintillator.quark'):
        print('Please run this script from the Scintillator Quark root directory.')
        sys.exit(1)

    # Make download and extraction directories.
    binary_path = os.path.join('build', 'binary-deps')
    install_ext = os.path.join('build', 'install-ext')
    os.makedirs(binary_path, exist_ok=True)
    os.makedirs(install_ext, exist_ok=True)

    if platform.system() == 'Linux':
        os_name = 'linux'
        needed_deps = ['swiftshader-ext']
    elif platform.system() == 'Darwin':
        os_name = 'osx'
        # needed_deps = ['ffmpeg-ext', 'vulkan-ext', 'swiftshader-ext']
        needed_deps = ['ffmpeg-ext']
    else:
        print('Unsupported operating system.')
        sys.exit(1)

    url_base = 'http://scintillator-synth-coverage.s3-website-us-west-1.amazonaws.com/binaries/'
    for dep in needed_deps:
        # Follow redirect to get current version of dependency.
        url_latest = url_base + dep + '/' + dep + '-' + os_name + '-latest.html'

        print('fetching latest for ' + dep + ' from ' + url_latest)
        req = urllib.request.urlopen(url_latest)
        # Response should be a simple html redirect, which we parse to extract url of the actual resource.
        resp = req.read(1024).decode('utf-8')
        begin_str = 'content="0; url='
        start = resp.find(begin_str) + len(begin_str)
        end = resp.find('"', start)
        url_file = resp[start:end]

        print('latest url for ' + dep + ' is ' + url_file)
        file_path = os.path.join(binary_path, url_file.split('/')[-1])
        sha_path = pathlib.PurePath(file_path).stem + '.sha256'
        url_sha = url_base + dep + '/' + sha_path
        sha_path = os.path.join(binary_path, sha_path)

        # Check if the file already exists, and download if not.
        if not os.path.exists(file_path):
            print('downloading ' + file_path + '...')
            with urllib.request.urlopen(url_file) as response, open(file_path, 'wb') as out_file:
                shutil.copyfileobj(response, out_file)
        else:
            print('skipping download of existing file ' + file_path)

        if not os.path.exists(sha_path):
            print('downloading key ' + sha_path + '...')
            with urllib.request.urlopen(url_sha) as response, open(sha_path, 'wb') as out_file:
                shutil.copyfileobj(response, out_file)
        else:
            print('skipping download of existing file ' + sha_path)

        # Check the hash of the downloaded file


if __name__ == "__main__":
    main(sys.argv[1:])
