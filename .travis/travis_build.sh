#!/bin/bash

# for executing in non travis environment

[ -z "$TRAVIS_OS_NAME" ] && TRAVIS_OS_NAME="$(uname -s | tr 'A-Z' 'a-z')"

# Detect number of processor cores

if [[ $TRAVIS_OS_NAME == 'osx' || $TRAVIS_OS_NAME == 'darwin' ]]; then
	procs=$(sysctl -n hw.ncpu | xargs)
elif [[ $TRAVIS_OS_NAME == 'linux' ]]; then
    procs=$(nproc)
else
    # For most modern systems, including the pi, this is a sane default
    procs=4
fi


# Compile hyperion

mkdir build || exit 1
cd build
cmake -DPLATFORM=x86-dev -DCMAKE_BUILD_TYPE=Debug .. || exit 2
make -j$(nproc) || exit 3


# Build the package on Linux

if [[ $TRAVIS_OS_NAME == 'linux' ]]; then
    make -j$(nproc) package || exit 4
fi

