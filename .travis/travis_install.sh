#!/bin/bash

# for executing in non travis environment
[ -z "$TRAVIS_OS_NAME" ] && TRAVIS_OS_NAME="$( uname -s | tr '[:upper:]' '[:lower:]' )"

# install osx deps for hyperion compile
if [[ $TRAVIS_OS_NAME == 'osx' || $TRAVIS_OS_NAME == 'darwin' ]]
then
	echo "Install OSX deps"
	time brew update
	time brew install qt5 || true
	time brew install python3 || true
	time brew install libusb || true
	time brew install cmake || true
	time brew install doxygen || true

# install linux deps for hyperion compile
elif [[ $TRAVIS_OS_NAME == 'linux' ]]
then
	echo "Install linux deps"
	#sudo apt-get -qq update
	#sudo apt-get install -qq -y qtbase5-dev libqt5serialport5-dev libusb-1.0-0-dev python3-dev libxrender-dev libavahi-core-dev libavahi-compat-libdnssd-dev doxygen expect
else
    echo "Unsupported platform: $TRAVIS_OS_NAME"
    exit 5
fi
