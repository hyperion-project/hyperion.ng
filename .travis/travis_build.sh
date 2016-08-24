#!/bin/bash

# for executing in non travis environment
[ -z "$TRAVIS_OS_NAME" ] && TRAVIS_OS_NAME="$( uname -s | tr '[:upper:]' '[:lower:]' )"


######################################
## COMPILE HYPERION

# compile hyperion on osx
if [[ $TRAVIS_OS_NAME == 'osx' || $TRAVIS_OS_NAME == 'darwin' ]]
then
	procs=$(sysctl -n hw.ncpu | xargs)
	echo "Processes: $procs"

	mkdir build || exit 1
    cd build
	cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=ON -Wno-dev .. || exit 2
	make -j$procs || exit 3
	# make -j$(nproc) package || exit 4 # currently osx(dmg) package creation not implemented
# compile hyperion on linux
elif [[ $TRAVIS_OS_NAME == 'linux' ]]
then
	mkdir build || exit 1
	cd build
	cmake -DPLATFORM=x86-dev -DCMAKE_BUILD_TYPE=Debug .. || exit 2
	make -j$(nproc) || exit 3
	make -j$(nproc) package || exit 4
else
    echo "Unsupported platform: $TRAVIS_OS_NAME"
    exit 5
fi

