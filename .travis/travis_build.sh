#!/bin/bash
# compile hyperion on osx
if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
	mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON .. && make -j$(nproc) package
fi

# compile hyperion on linux
if [[ $TRAVIS_OS_NAME == 'linux' ]]; then
	mkdir build && cd build && cmake -DPLATFORM=x86 -DCMAKE_BUILD_TYPE=Release -DENABLE_AMLOGIC=ON -DENABLE_TESTS=ON -DENABLE_SPIDEV=ON .. && make -j$(nproc) package
fi
