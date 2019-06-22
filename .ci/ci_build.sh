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

# set environment variables if not exists
[ -z "${BUILD_TYPE}" ] && BUILD_TYPE="Debug"

# Determine cmake build type; tag builds are Release, else Debug
if [[ $BUILD_SOURCEBRANCH == *"refs/tags"* ]]; then
	BUILD_TYPE=Release
fi

# Determie -dev appends to platform;
# Commented because tests are currently broken
# [ "${TRAVIS_EVENT_TYPE:-}" != 'cron' -a -z "${TRAVIS_TAG:-}" ] && PLATFORM=${PLATFORM}-dev

# Build the package on osx or linux
if [[ "$CI_NAME" == 'osx' || "$CI_NAME" == 'darwin' ]]; then
	# compile prepare
	mkdir build || exit 1
	mkdir ${CI_BUILD_DIR}/deploy || exit 1
	cd build
	cmake -DPLATFORM="osx" -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=/usr/local ../ || exit 2
	make -j $(sysctl -n hw.ncpu) package || exit 3
	exit 0;
	exit 1 || { echo "---> Hyperion compilation failed! Abort"; exit 4; }
elif [[ "$CI_NAME" == 'linux' ]]; then
	echo "Compile Hyperion with DOCKER_TAG = ${DOCKER_TAG} and friendly name DOCKER_NAME = ${DOCKER_NAME}"
	# take ownership of deploy dir
	mkdir ${CI_BUILD_DIR}/deploy

	# run docker
	docker run --rm \
		-v "${CI_BUILD_DIR}/deploy:/deploy" \
		-v "${CI_BUILD_DIR}:/source:ro" \
		hyperionproject/hyperion-ci:$DOCKER_TAG \
		/bin/bash -c "mkdir hyperion.ng && cp -r source/. /hyperion.ng &&
		cd /hyperion.ng && mkdir build && cd build &&
		cmake -DPLATFORM=${PLATFORM} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DDOCKER_PLATFORM=${DOCKER_TAG} ../ || exit 2 &&
		make -j $(nproc) package || exit 3 &&
		cp /hyperion.ng/build/bin/h* /deploy/ 2>/dev/null || : &&
		cp /hyperion.ng/build/Hyperion.NG-* /deploy/ 2>/dev/null || : &&
		exit 0;
		exit 1 " || { echo "---> Hyperion compilation failed! Abort"; exit 4; }

	# overwrite file owner to current user
	sudo chown -fR $(stat -c "%U:%G" ${CI_BUILD_DIR}/deploy) ${CI_BUILD_DIR}/deploy
fi
