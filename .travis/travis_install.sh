#!/bin/bash

# install osx deps for hyperion compile
if [[ $TRAVIS_OS_NAME == 'osx' ]]
then
	echo "Install OSX deps"
	time brew update
	time brew install qt5 || true
	time brew install libusb || true
	time brew install cmake || true
	time brew install doxygen || true
fi

# install linux deps for hyperion compile
if [[ $TRAVIS_OS_NAME == 'linux' ]]
then
	echo "Install linux deps"
	sudo apt-get -qq update
	sudo apt-get install -qq -y qtbase5-dev libqt5serialport5-dev libusb-1.0-0-dev python-dev libxrender-dev libavahi-core-dev libavahi-compat-libdnssd-dev doxygen
fi

