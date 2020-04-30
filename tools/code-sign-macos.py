#!/user/bin/python3

# Script for automatically signing and getting Apple to notarize the signed binary. It is assumed that the required
# certificate is already installed in a keychain and the keychain is unlocked. Needs to be run from the Scintillator
# root directory, and is only going to work on MacOS. Arguments are Apple Dev username, app-specific password, and
# signing identity.

import os
import re
import shutil
import subprocess
import sys
import time

def main(argv):
    if len(argv) != 3:
        printf('usage:')
        printf('  python3 tools/code-sign-macos.py <apple dev username> <app-specific password> <signing identity>')
        sys.exit(1)

    appleDevID = argv[0]
    appleDevPassword = argv[1]
    signIdentity = argv[2]

    if not os.path.exists('Scintillator.quark'):
        print('Please run this script from the Scintillator Quark root directory.')
        sys.exit(1)

    print('*** Checking certificate.')
    # Check that the certificate and keychain have been adequately configured.
    certCheck = subprocess.run(['security', 'find-identity', '-v'], check=True,
            stdout=subprocess.PIPE).stdout.decode('utf-8')
    if signIdentity not in certCheck:
        print("Couldn't find the signing cert in the keychain.")
        sys.exit(1)

    # Check that the binary has been built and is present.
    if not os.path.exists('bin/scinsynth.app'):
        print('Unable to find Scintillator binary.')
        sys.exit(1)

    print('*** Signing binary.')
    # Copy the entitlements list into the binary.
    shutil.copyfile('src/macos/entitlements.plist', 'bin/scinsynth.app/Contents/entitlements.plist')

    codesign = ['codesign', '--deep', '--force', '--verify', '--verbose', '--timestamp', '--entitlements',
            'bin/scinsynth.app/Contents/entitlements.plist', '--sign', signIdentity]
    runtime = ['--options', 'runtime']

    # Sign app and binary.
    subprocess.run(codesign + runtime + ['bin/scinsynth.app'], check=True)
    subprocess.run(codesign + runtime + ['bin/scinsynth.app/Contents/MacOS/scinsynth'], check=True)

    # Sign all dynamic libraries.
    dylibs = subprocess.run(['find', 'bin/scinsynth.app', '-type', 'f', '-name', '*.dylib'], check=True,
            stdout=subprocess.PIPE).stdout.decode('utf-8').split()
    for lib in dylibs:
        subprocess.run(codesign + runtime + [lib], check=True)

    # Zip binary
    print('*** Compressing binary to zipfile')
    subprocess.run(['ditto', '-c', '-k', '--rsrc', '--keepParent', 'bin/scinsynth.app', 'bin/scinsynth.app.zip'],
            check=True)

    # Upload app for notarization.
    print('*** Uploading app for notarization.')
    notarize = subprocess.run(['xcrun', 'altool', '--notarize-app', '-t', 'osx', '-f', 'bin/scinsynth.app.zip',
        '--primary-bundle-id', 'org.scintillatorsynth.scinsynth', '-u', appleDevID, '-p', appleDevPassword],
        check=True, stdout=subprocess.PIPE).stdout.decode('utf-8')
    uuid = re.search(r"RequestUUID = ([0-9a-f-]+)", notarize).groups()[0]

    # Polling loop for notarization success.
    print('*** Upload successful, uuid ' + uuid + '. Starting poll loop.')
    done = False
    counter = 0
    while not done:
        for i in range(0, 5):
            print('  log keepalive, ' + str(5 - i) + ' minute(s) remain.')
            time.sleep(60)
        print('*** Checking notarization status:')
        notarize = subprocess.run(['xcrun', 'altool', '--notarization-info', uuid, '-u', appleDevID, '-p',
            appleDevPassword], check=True, stdout=subprocess.PIPE).stdout.decode('utf-8')
        print(notarize) # Useful for log trail as it has the Apple log URL
        status = re.search(r"Status: ([A-Za-z0-9]+)", notarize).groups()[0]
        if status == 'success':
            done = True
        else:
            counter = counter + 1
            if counter == 12:
                print('** Failed to detect successful notarization. Quitting.')
                sys.exit(1)

    print('*** Successful notarization detected, stapling binary.')
    subprocess.run(['spctl', '-a', '-v', 'bin/scinsynth.app'], check=True)

    # Remove temporary zip and recreate with signed binary.
    print('*** Recreating application zipfile.')
    os.remove('bin/scinsynth.app.zip')
    subprocess.run(['ditto', '-c', '-k', '--rsrc', '--keepParent', 'bin/scinsynth.app', 'bin/scinsynth.app.zip'],
            check=True)

    print('*** Signing the zipfile.')
    subprocess.run(codesign + ['bin/scinsynth.app.zip'])

    print('*** Done!')
    sys.exit(0)

if __name__ == '__main__':
    main(sys.argv[1:])
