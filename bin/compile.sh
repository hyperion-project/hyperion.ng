#!/bin/sh

TARGET=${1:-hyperion}
CFG="${2:-Release}"
INST="$( [ "${3:-}" = "install" ] && echo true || echo false )"

sudo apt-get update
sudo apt-get install git cmake build-essential qtbase5-dev libqt5serialport5-dev libusb-1.0-0-dev python3-dev libxrender-dev libavahi-core-dev libavahi-compat-libdnssd-dev  || exit 1

if [ -e /dev/vc-cma -a -e /dev/vc-mem ]
then
	sudo apt-get install libraspberrypi-dev || exit 1
fi

git clone --recursive https://github.com/hyperion-project/hyperion.ng.git ${TARGET} || exit 1

rm -rf  $TARGET/build
mkdir -p $TARGET/build           || exit 1
cd $TARGET/build                 || exit 1
cmake -DCMAKE_BUILD_TYPE=$CFG .. || exit 1
make -j $(nproc)                 || exit 1

# optional: install into your system
$INST && sudo make install/strip
echo "to uninstall (not very well tested, please keep that in mind):"
echo "   sudo make uninstall"
