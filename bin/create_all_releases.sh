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
	
	rm -rf build-${RELEASE}
	mkdir -p build-${RELEASE}
	rm -rf deploy/${RELEASE}
	mkdir -p deploy/${RELEASE}

	cd  build-${RELEASE}

	cmake -DCMAKE_INSTALL_PREFIX=/usr -DPLATFORM=${PLATFORM} -DCMAKE_BUILD_TYPE=Release -Wno-dev $@ .. || exit 1
	#cmake  -DCMAKE_INSTALL_PREFIX=/usr -DPLATFORM=${PLATFORM} -DCMAKE_BUILD_TYPE=Debug $@ .. || exit 1
	make -j $(nproc)  || exit 1
	strip bin/*
	make package -j $(nproc)
	mv Hyperion-* ../deploy/${RELEASE}
	cd ..
	bin/create_release.sh . ${RELEASE}
}

#export QTDIR="/opt/Qt/6.2.2/gcc_64"
#export QTDIR="/opt/Qt/5.15.2/gcc_64"

CMAKE_PROTOC_FLAG="-DIMPORT_PROTOC=../build-x86x64/protoc_export.cmake"
CMAKE_FLATC_FLAG="-DIMPORT_FLATC=../build-x86x64/flatc_export.cmake"

make_release x86x64 x11 $@
#make_release x32     x11   $@ -DCMAKE_TOOLCHAIN_FILE="../cmake/Toolchain-x32.cmake" ${CMAKE_PROTOC_FLAG} ${CMAKE_FLATC_FLAG}
#make_release rpi     rpi   $@ -DCMAKE_TOOLCHAIN_FILE="../cmake/Toolchain-rpi.cmake"  ${CMAKE_PROTOC_FLAG} ${CMAKE_FLATC_FLAG}
#make_release wetek   wetek $@ -DCMAKE_TOOLCHAIN_FILE="../cmake/Toolchain-rpi.cmake"  ${CMAKE_PROTOC_FLAG} ${CMAKE_FLATC_FLAG} 
#make_release imx6    imx6  $@ -DCMAKE_TOOLCHAIN_FILE="../cmake/Toolchain-imx6.cmake" ${CMAKE_PROTOC_FLAG} ${CMAKE_FLATC_FLAG} 
