#!/bin/sh

# make_release <release name> <platform> [<cmake args ...>]
make_release()
{
	echo
	echo "--- build release for $1 ---"
	echo
	RELEASE=$1
	PLATFORM=$2
	shift 2

	mkdir -p build-${RELEASE}
	mkdir -p deploy/${RELEASE}
	cd  build-${RELEASE}
	cmake -DCMAKE_INSTALL_PREFIX=/usr -DPLATFORM=${PLATFORM} $@ -DCMAKE_BUILD_TYPE=Release -Wno-dev .. || exit 1
	make -j $(nproc)  || exit 1
	#strip bin/*
	make package -j $(nproc)
	mv hyperion-*-ambilight.* ../deploy/${RELEASE}
	cd ..
	bin/create_release.sh . ${RELEASE}
}

export PATH="$PATH:$HOME/raspberrypi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin"
CMAKE_PROTOC_FLAG="-DIMPORT_PROTOC=../build-x86x64/protoc_export.cmake"

make_release x86x64  x86
#make_release x32     x86 ${CMAKE_PROTOC_FLAG}
make_release rpi     rpi-pwm  -DCMAKE_TOOLCHAIN_FILE="../cmake/Toolchain-rpi.cmake" ${CMAKE_PROTOC_FLAG}
make_release wetek   wetek  -DCMAKE_TOOLCHAIN_FILE="../cmake/Toolchain-rpi.cmake" ${CMAKE_PROTOC_FLAG}
#make_release imx6   imx6 -DCMAKE_TOOLCHAIN_FILE="../cmake/Toolchain-imx6.cmake" ${CMAKE_PROTOC_FLAG}


