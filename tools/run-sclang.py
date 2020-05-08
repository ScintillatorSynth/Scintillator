#!/usr/bin/python3
# Inspired by the SuperCollider script testsuite/sclang/launch_test.py

import sys
import os
import subprocess
import time
import select

def main(argv):
    sclang_path = argv[0]
    assert os.path.exists(sclang_path)

    env = dict(os.environ)
    xvfb = None
    if sys.platform == 'linux':
        env['QT_PLATFORM_PLUGIN'] = 'offscreen'
        env['DISPLAY'] = ':99.0'
        xvfb = subprocess.Popen(['Xvfb', ':99', '-ac', '-screen', '0', '1280x1024x24'])

    print('sclang command line: ' + sclang_path + ' ' + ' '.join(argv[1:]))
    proc = subprocess.Popen([sclang_path] + argv[1:], stdout=subprocess.PIPE, env=env)

    timeout = 300
    done_string = "*** SCRIPT OK ***"
    error_string = "*** SCRIPT FAILED ***"
    start_time = time.time()
    output = ""
    while not ((done_string in output) or (error_string in output)):
        if time.time() > (start_time + timeout):
            output = error_string
            break
        output = proc.stdout.readline().decode('utf-8').replace('\r', '')
        print(output, end="")


    if xvfb:
        xvfb.terminate()
    if error_string in output:
        print('script failure caught, exiting.')
        sys.exit(-1)

    print('script success caught, exiting.')
    sys.exit(0)

if __name__ == "__main__":
    main(sys.argv[1:])

