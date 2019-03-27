#!/bin/bash

# for executing in non travis environment
[ -z "$TRAVIS_OS_NAME" ] && TRAVIS_OS_NAME="$(uname -s | tr '[:upper:]' '[:lower:]')"

PLATFORM=x86
BUILD_TYPE=Debug
PACKAGES=""

# Detect number of processor cores
# default is 4 jobs
if [[ "$TRAVIS_OS_NAME" == 'osx' || "$TRAVIS_OS_NAME" == 'darwin' ]]
then
	JOBS=$(sysctl -n hw.ncpu)
	PLATFORM=osx
elif [[ "$TRAVIS_OS_NAME" == 'linux' ]]
then
	JOBS=$(nproc)
fi
echo "compile jobs: ${JOBS:=4}"

# Determine cmake build type; tag builds are Release, else Debug
[ -n "${TRAVIS_TAG:-}" ] && BUILD_TYPE=Release

# Determine package creation; True for cron and tag builds
[ "${TRAVIS_EVENT_TYPE:-}" == 'cron' ] || [ -n "${TRAVIS_TAG:-}" ] && PACKAGES=package

# Determie -dev appends to platform;
[ "${TRAVIS_EVENT_TYPE:-}" != 'cron' -a -z "${TRAVIS_TAG:-}" ] && PLATFORM=${PLATFORM}-dev

# Build the package on osx
if [[ "$TRAVIS_OS_NAME" == 'osx' || "$TRAVIS_OS_NAME" == 'darwin' ]]
then
	# compile prepare
	mkdir build || exit 1
	cd build
	cmake -DPLATFORM=$PLATFORM -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_INSTALL_PREFIX=/usr .. || exit 2
	make -j ${JOBS} || exit 3
fi

# Build the package with docker
if [[ $TRAVIS_OS_NAME == 'linux' ]]
then
	echo "Compile Hyperion with DOCKER_TAG = ${DOCKER_TAG} and friendly name DOCKER_NAME = ${DOCKER_NAME}"
	# take ownership of deploy dir
	mkdir $TRAVIS_BUILD_DIR/deploy
	# run docker
	docker run --rm \
		-v "${TRAVIS_BUILD_DIR}/deploy:/deploy" \
		-v "${TRAVIS_BUILD_DIR}:/source:ro" \
		hyperionorg/hyperion-ci:$DOCKER_TAG \
		/bin/bash -c "mkdir build && cp -r /source/. /build &&
		cd /build && mkdir build && cd build &&
		cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} .. || exit 2 &&
		make -j $(nproc) ${PACKAGES} || exit 3 &&
		echo '---> Copy binaries and packages to host folder: ${TRAVIS_BUILD_DIR}/deploy' &&
		cp -v /build/build/bin/h* /deploy/ 2>/dev/null || : &&
		cp -v /build/build/Hyperion-* /deploy/ 2>/dev/null || : &&
		exit 0;
		exit 1 " || { echo "---> Hyperion compilation failed! Abort"; exit 4; }

	# overwrite file owner to current user
	sudo chown -fR $(stat -c "%U:%G" $TRAVIS_BUILD_DIR/deploy) $TRAVIS_BUILD_DIR/deploy
fi
