#!/bin/bash
# install osx deps for hyperion compile
if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
	echo "Install OSX deps"
	brew update
	brew install qt5
	brew install libusb
	brew install cmake
fi

# install linux deps for hyperion compile
if [[ $TRAVIS_OS_NAME == 'linux' ]]; then
	echo "Install linux deps"
	sudo apt-get -qq update ; sudo apt-get install -qq -y qtbase5-dev libqt5serialport5-dev libusb-1.0-0-dev python-dev libxrender-dev libavahi-core-dev libavahi-compat-libdnssd-dev
fi

