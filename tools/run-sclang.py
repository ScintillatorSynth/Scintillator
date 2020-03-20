#!/usr/bin/python3
# Inspired by the SuperCollider script testsuite/sclang/launch_test.py

import fcntl
import sys
import os
import subprocess
import time
import select

def non_block_read(output):
    fd = output.fileno()
    fl = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)
    try:
        return output.readline().decode("utf-8")
    except:
        return ""

def main(argv):
    sclang_path = argv[0]
    env = dict(os.environ)
    xvfb = None
    if sys.platform == 'linux':
        env['QT_PLATFORM_PLUGIN'] = 'offscreen'
        env['DISPLAY'] = ':99.0'
        xvfb = subprocess.Popen(['Xvfb', ':99', '-ac', '-screen', '0', '1280x1024x24'])
    proc = subprocess.Popen([sclang_path] + argv[1:],
        stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.PIPE,
        env=env)

    timeout = 300
    done_string = "*** SCRIPT OK ***"
    error_string = "*** SCRIPT FAILED ***"
    start_time = time.time()
    output = ""
    while not ((done_string in output) or (error_string in output)):
        if time.time() > (start_time + timeout):
            output = error_string
            break

        output = non_block_read(proc.stdout)
        error = non_block_read(proc.stderr)
        if error:
            print("ERROR:" + error, end="")
        elif output:
            print(output, end="")

        time.sleep(0.01)

    if xvfb:
        xvfb.terminate()
    if error_string in output:
        sys.exit(-1)
    sys.exit(0)

if __name__ == "__main__":
    main(sys.argv[1:])

