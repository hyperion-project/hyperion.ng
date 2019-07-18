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

	rm -r hyperion.ng-${RELEASE}
	mkdir -p hyperion.ng-${RELEASE}
	rm -r deploy/${RELEASE}
	mkdir -p deploy/${RELEASE}

	cd  hyperion.ng-${RELEASE}
	cmake -DCMAKE_INSTALL_PREFIX=/usr -DPLATFORM=${PLATFORM} $@ -DCMAKE_BUILD_TYPE=Release -Wno-dev .. || exit 1
	make -j $(nproc)  || exit 1
	#strip bin/*
	make package -j $(nproc)
	mv Hyperion.NG-* ../deploy/${RELEASE}
	cd ..
	bin/create_release.sh . ${RELEASE}
}

CMAKE_PROTOC_FLAG="-DIMPORT_PROTOC=../hyperion.ng-x86x64/protoc_export.cmake"
CMAKE_FLATC_FLAG="-DIMPORT_FLATC=../hyperion.ng-x86x64/flatc_export.cmake"

make_release x86x64  x86
#make_release x32     x86   -DCMAKE_TOOLCHAIN_FILE="../cmake/Toolchain-x32.cmake" ${CMAKE_PROTOC_FLAG} ${CMAKE_FLATC_FLAG}
make_release rpi     rpi   -DCMAKE_TOOLCHAIN_FILE="../cmake/Toolchain-rpi.cmake"  ${CMAKE_PROTOC_FLAG} ${CMAKE_FLATC_FLAG}
#make_release wetek   wetek -DCMAKE_TOOLCHAIN_FILE="../cmake/Toolchain-rpi.cmake"  ${CMAKE_PROTOC_FLAG} ${CMAKE_FLATC_FLAG} 
#make_release imx6    imx6  -DCMAKE_TOOLCHAIN_FILE="../cmake/Toolchain-imx6.cmake" ${CMAKE_PROTOC_FLAG} ${CMAKE_FLATC_FLAG} 


