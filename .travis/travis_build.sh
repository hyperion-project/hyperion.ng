#!/bin/bash
# compile hyperion on osx
if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
	cmake . -DCMAKE_PREFIX_PATH=/usr/local/Cellar/qt5/5.6.1-1
	mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON -Wno-dev .. && make -j$(nproc) package
fi

# compile hyperion on linux
if [[ $TRAVIS_OS_NAME == 'linux' ]]; then
	mkdir build && cd build && cmake -DPLATFORM=x86 -DCMAKE_BUILD_TYPE=Release -DENABLE_AMLOGIC=ON -DENABLE_TESTS=ON -DENABLE_SPIDEV=ON -Wno-dev .. && make -j$(nproc) package
fi
