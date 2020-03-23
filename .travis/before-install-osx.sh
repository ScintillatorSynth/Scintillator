#!/bin/sh

export HOMEBREW_NO_ANALYTICS=1

brew update
brew unlink python@2
brew install ccache doxygen shtool supercollider

# according to https://docs.travis-ci.com/user/caching#ccache-cache
export PATH="/usr/local/opt/ccache/libexec:$PATH"

# To get less noise in xcode output, as some builds are terminated for exceeding maximum log length.
gem install xcpretty-travis-formatter

