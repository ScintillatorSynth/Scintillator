#!/usr/bin/python3

# Script, primarily for Travis-CI, that fetches a pre-compiled build of SuperColider so we can use sclang to run
# integration and unit testing on the server. It accepts one argument, which is the path where it should download and
# extract the os-specific version of SuperCollider.

import os
import pathlib
import platform
import shutil
import subprocess
import sys
import urllib.request

def main(argv):
    if len(argv) != 1:
        print('usage:')
        print('python3 tools/fetch-sclang.py <path to install sc in>')
        sys.exit(1)

    sclang_path = argv[0]
    os.makedirs(sclang_path, exist_ok=True)

    if platform.system() == 'Linux':
        os_name = 'linux'
    elif platform.system() == 'Darwin':
        os_name = 'osx'
    elif platform.system() == 'Windows':
        os_name = 'windows'
    else:
        print('Unsupported operating system.')
        sys.exit(1)

    url_base = 'http://scintillator-synth-coverage.s3-website-us-west-1.amazonaws.com/binaries/sclang-ext/'
    url_latest = url_base + 'sclang-ext-' + os_name + '-latest.html'

    print('fetching latest sclang from ' + url_latest)
    req = urllib.request.urlopen(url_latest)
    resp = req.read().decode('utf-8')
    begin_str = 'content="0; url='
    start = resp.find(begin_str) + len(begin_str)
    end = resp.find('"', start)
    url_file = resp[start:end]

    print('latest url for sclang-ext is ' + url_file)
    file_path = os.path.join(sclang_path, url_file.split('/')[-1])
    sha_file = pathlib.PurePath(file_path).stem + '.sha256'
    url_sha = url_base + sha_file
    sha_path = os.path.join(sclang_path, sha_file)

    print('downloading ' + file_path + '...')
    with urllib.request.urlopen(url_file) as response, open(file_path, 'wb') as out_file:
        shutil.copyfileobj(response, out_file)
    print('downloading key ' + sha_path + '...')
    with urllib.request.urlopen(url_sha) as response, open(sha_path, 'wb') as out_file:
        shutil.copyfileobj(response, out_file)

    # Check the hash of the downloaded file
    if os_name == 'windows':
        hash_check = subprocess.run(['certutil', '-hashfile', file_path, 'SHA256'], stdout=subprocess.PIPE);
        hash_result = hash_check.stdout.decode('utf-8').split('\n')[1].strip();            
        hash_expected = open(sha_path, 'r').read().strip()
        if hash_result != hash_expected:
            print ('file ' + file_path + ' failed hash! Expecting "' + hash_expected
                + '" got "' + hash_result + '"')
            sys.exit(1)
    else:
        hash_check = subprocess.run(['shasum', '-c', sha_file], cwd=sclang_path, stdout=subprocess.PIPE)
        hash_result = hash_check.stdout.decode('utf-8')
        if hash_result[-3:-1] != 'OK':
            print('file ' + file_path + ' failed hash!')
            sys.exit(1)

    # Extract file
    print('extracting ' + file_path + ' to ' + sclang_path)
    extract_file = subprocess.run(['tar', 'xzf', file_path, '-C', sclang_path ])

if __name__ == '__main__':
    main(sys.argv[1:])
