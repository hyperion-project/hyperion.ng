#!/bin/bash -v

# for executing in non travis environment
[ -z "$TRAVIS_OS_NAME" ] && TRAVIS_OS_NAME="$( uname -s | tr '[:upper:]' '[:lower:]' )"


######################################
## COMPILE HYPERION

# compile hyperion on osx
if [[ $TRAVIS_OS_NAME == 'osx' ]]
then
    qt5_path=$(find /usr/local/Cellar/qt5 -maxdepth 1 | sort -nr | head -n 1 | xargs)
	mkdir build || exit 1
	cd build
	cmake -DCMAKE_PREFIX_PATH="$qt5_path" -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON -Wno-dev .. || exit 2
	cd ..
	make -j$(sysctl -n hw.ncpu) || exit 3
	# make -j$(nproc) package || exit 4 # currently osx(dmg) package creation not implemented
fi

# compile hyperion on linux
if [[ $TRAVIS_OS_NAME == 'linux' ]]
then
	mkdir build || exit 1
	cd build
	cmake -DPLATFORM=x86-dev -DCMAKE_BUILD_TYPE=Release .. || exit 2
	make -j$(nproc) || exit 3
	make -j$(nproc) package || exit 4
fi

