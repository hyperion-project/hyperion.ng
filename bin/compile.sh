#!/bin/sh

# exit on error
set -e

TARGET=${1:-hyperion}
CFG="${2:-Release}"
INST="$( [ "${3:-}" = "install" ] && echo true || echo false )"

sudo apt-get update
sudo apt-get install                 \
	git                          \
	cmake                        \
	build-essential              \
	qtbase5-dev                  \
	libqt5serialport5-dev        \
	libusb-1.0-0-dev             \
	libturbojpeg0-dev            \
	libjpeg-dev                  \
	python3-dev                  \
	libcec-dev                   \
	libxcb-util0-dev             \
	libxcb-randr0-dev            \
	libxrandr-dev                \
	libxrender-dev               \
	libavahi-core-dev            \
	libavahi-compat-libdnssd-dev \
	libssl-dev                   \
	libqt5sql5-sqlite            \
	libqt5svg5-dev               \

if [ -e /dev/vc-cma -a -e /dev/vc-mem ]
then
	sudo apt-get install libraspberrypi-dev
fi

git clone --recursive https://github.com/hyperion-project/hyperion.ng.git ${TARGET}

rm -rf $TARGET/build
mkdir -p $TARGET/build
cd $TARGET/build
cmake -DCMAKE_BUILD_TYPE=$CFG ..
make -j $(nproc)

# optional: install into your system
$INST && sudo make install/strip
echo "to uninstall (not very well tested, please keep that in mind):"
echo "   sudo make uninstall"
