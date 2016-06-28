#!/bin/bash
# install osx deps for hyperion compile
if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
	sudo brew install qt5-mac cmake libusb
fi

# install linux deps for hyperion compile
if [[ $TRAVIS_OS_NAME == 'linux' ]]; then
	sudo apt-get -qq update ; sudo apt-get install -qq -y qtbase5-dev libqt5serialport5-dev libusb-1.0-0-dev python-dev libxrender-dev libavahi-core-dev libavahi-compat-libdnssd-dev
fi

