#!/bin/sh

if [[ -z $TRAVIS_TAG ]]; then
    mkdir -p $HOME/builds
    cd $TRAVIS_BUILD_DIR/bin && tar czf $HOME/builds/scinsynth.app.$TRAVIS_COMMIT.tgz scinsynth.app
    shasum -a 256 -b $HOME/builds/scinsynth.app.$TRAVIS_COMMIT.tgz > $HOME/builds/scinsynth.app.$TRAVIS_COMMIT.sha256
    export S3_URL='http://scintillator-synth-coverage.s3-website-us-west-1.amazonaws.com/builds/scinsynth.app.'$TRAVIS_COMMIT'.tgz'
    export FWD_HTML='<html><head><meta http-equiv="refresh" content="0; url='$S3_URL'" /></head></html>'
    echo $FWD_HTML > $HOME/builds/scinsynth.app.latest.html
else
    # First we must sign the binary. Create a keychain for storing the cert in.
    security create-keychain -p $OSX_BUILD_KEYCHAIN_PWD build.keychain

    # Add to the existing list of keychains. There's no append so we must get the list and then add.
    keychains=$(security list-keychains -d user)
    keychainNames=();
    for keychain in keychains
    do
        basename=$(basename "$keychain")
        keychainName=${basename::${#basename}-4}
        keychainNames+=("$keychainName")
    done
    security list-keychains -s "${keychainNames[@]}" build.keychain

    # Unlock our new keychain so we don't get password prompts for it.
    security unlock-keychain -p $OSX_BUILD_KEYCHAIN_PWD build.keychain

    # Unset the lock timeout so the script doesn't break having to re-auth the keychain.
    security set-keychain-settings -u build.keychain

    # Decode the certificate from the environment variable and add to keychain.
    echo $CERTIFICATE_OSX_P12 | base64 --decode > $HOME/certificate.p12
    security import $HOME/certificate.p12 -k build.keychain -P $OSX_CERT_PWD

    # re-unlock our new keychain so we don't get password prompts for it.
    security unlock-keychain -p $OSX_BUILD_KEYCHAIN_PWD build.keychain

    # Run the script to sign and then notarize the build.
    echo "Certificate installed, running signing script"
    cd $TRAVIS_BUILD_DIR
    python3 tools/code-sign-macos.py "$APPLE_DEV_USERNAME" $APPLE_DEV_PWORD "$APPLE_SIGNING_IDENTITY" || exit 1

    # Finish up staging release directory.
    mkdir -p $HOME/releases/$TRAVIS_TAG
    cp $TRAVIS_BUILD_DIR/bin/scinsynth.app.zip $HOME/releases/$TRAVIS_TAG/.
    cd $HOME/releases/$TRAVIS_TAG
    shasum -a 256 -b scinsynth.app.zip > scinsynth.app.zip.sha256
    security delete-keychain build.keychain
fi
