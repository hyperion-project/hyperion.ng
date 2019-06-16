#!/bin/bash

# detect CI
if [ -n "${TRAVIS-}" ]; then
	# Travis-CI
	CI_NAME="$(echo "$TRAVIS_OS_NAME" | tr '[:upper:]' '[:lower:]')"
	CI_BUILD_DIR="$TRAVIS_BUILD_DIR"
elif [ "$SYSTEM_COLLECTIONID" != "" ]; then
	# Azure Pipelines
	CI_NAME="$(echo "$AGENT_OS" | tr '[:upper:]' '[:lower:]')"
	CI_BUILD_DIR="$BUILD_SOURCESDIRECTORY"
else
	# for executing in non ci environment
	CI_NAME="$(uname -s | tr '[:upper:]' '[:lower:]')"
fi

# set environment variables
BUILD_TYPE="Debug"
[ -z "${PACKAGES}" ] && PACKAGES=""
[ -z "${PLATFORM}" ] && PLATFORM="x11"

# Detect number of processor cores
# default is 4 jobs
if [[ "$CI_NAME" == 'osx' || "$CI_NAME" == 'darwin' ]]; then
	JOBS=$(sysctl -n hw.ncpu)
	PLATFORM=osx
elif [[ "$CI_NAME" == 'linux' ]]; then
	JOBS=$(nproc)
fi
echo "compile jobs: ${JOBS:=4}"

# Determine cmake build type; tag builds are Release, else Debug
if [ -n "${TRAVIS_TAG:-}" ] || [[ $BUILD_SOURCEBRANCH == *"refs/tags"* ]]; then
	BUILD_TYPE=Release
fi

# Determine package creation; True for cron/schedule and tag builds
if [ "${TRAVIS_EVENT_TYPE:-}" == 'cron' ] || [ -n "${TRAVIS_TAG:-}" ] || [[ $BUILD_REASON == "Schedule" ]] || [[ $BUILD_SOURCEBRANCH == *"refs/tags"* ]]; then
	PACKAGES="package"
fi

# Determie -dev appends to platform;
# Commented because tests are currently broken
# [ "${TRAVIS_EVENT_TYPE:-}" != 'cron' -a -z "${TRAVIS_TAG:-}" ] && PLATFORM=${PLATFORM}-dev

# Build the package on osx or docker for linux
if [[ "$CI_NAME" == 'osx' || "$CI_NAME" == 'darwin' ]]; then
	# compile prepare
	mkdir build || exit 1
	mkdir ${CI_BUILD_DIR}/deploy || exit 1
	cd build
	cmake -DPLATFORM=$PLATFORM -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_INSTALL_PREFIX:PATH=/usr/local ../ || exit 2
	make -j ${JOBS} || exit 3
	if [[ "$PACKAGES" == 'package' ]]; then
		sudo cpack
	fi
	echo "---> Copy binaries and packages to folder: ${CI_BUILD_DIR}/deploy"
	cp -v ${CI_BUILD_DIR}/build/bin/h* ${CI_BUILD_DIR}/deploy/ 2>/dev/null || : &&
	cp -v ${CI_BUILD_DIR}/build/Hyperion-* ${CI_BUILD_DIR}/deploy/ 2>/dev/null || : &&
	exit 0;
elif [[ "$CI_NAME" == 'linux' ]]; then
	echo "Compile Hyperion with DOCKER_TAG = ${DOCKER_TAG} and friendly name DOCKER_NAME = ${DOCKER_NAME}"
	# take ownership of deploy dir
	mkdir ${CI_BUILD_DIR}/deploy

	# run docker
	docker run --rm \
		-v "${CI_BUILD_DIR}/deploy:/deploy" \
		-v "${CI_BUILD_DIR}:/source:ro" \
		hyperionproject/hyperion-ci:$DOCKER_TAG \
		/bin/bash -c "mkdir build && cp -r source/. /build &&
		cd /build && mkdir build && cd build &&
		cmake -DPLATFORM=${PLATFORM} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} .. || exit 2 &&
		make -j ${JOBS} ${PACKAGES} || exit 3 &&
		echo '---> Copy binaries and packages to host folder: ${CI_BUILD_DIR}/deploy' &&
		cp -v /build/build/bin/h* /deploy/ 2>/dev/null || : &&
		cp -v /build/build/Hyperion-* /deploy/ 2>/dev/null || : &&
		exit 0;
		exit 1 " || { echo "---> Hyperion compilation failed! Abort"; exit 4; }

	# overwrite file owner to current user
	sudo chown -fR $(stat -c "%U:%G" ${CI_BUILD_DIR}/deploy) ${CI_BUILD_DIR}/deploy
fi
