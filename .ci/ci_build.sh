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
elif [ "$HOME" != "" ]; then
	# GitHub Actions
	CI_NAME="$(uname -s | tr '[:upper:]' '[:lower:]')"
	CI_BUILD_DIR="$GITHUB_WORKSPACE"
else
	# for executing in non ci environment
	CI_NAME="$(uname -s | tr '[:upper:]' '[:lower:]')"
fi

# set environment variables if not exists
[ -z "${BUILD_TYPE}" ] && BUILD_TYPE="Debug"

# Determine cmake build type; tag builds are Release, else Debug (-dev appends to platform)
if [[ $BUILD_SOURCEBRANCH == *"refs/tags"* || $GITHUB_REF == *"refs/tags"* ]]; then
	BUILD_TYPE=Release
else
	PLATFORM=${PLATFORM}-dev
fi

# Build the package on osx or linux
if [[ "$CI_NAME" == 'osx' || "$CI_NAME" == 'darwin' ]]; then
	# compile prepare
	mkdir build || exit 1
	cd build
	cmake -DPLATFORM=${PLATFORM} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=/usr/local ../ || exit 2
	make -j $(sysctl -n hw.ncpu) || exit 3 # Notes: The package creation is currently not supported because of strange errors.
	cd ${CI_BUILD_DIR} && source /${CI_BUILD_DIR}/test/testrunner.sh || exit 4
	exit 0;
	exit 1 || { echo "---> Hyperion compilation failed! Abort"; exit 5; }
elif [[ "$CI_NAME" == 'linux' ]]; then
	echo "Compile Hyperion with DOCKER_TAG = ${DOCKER_TAG} and friendly name DOCKER_NAME = ${DOCKER_NAME}"
	# take ownership of deploy dir
	mkdir ${CI_BUILD_DIR}/deploy

	# run docker
	docker run --rm \
		-v "${CI_BUILD_DIR}/deploy:/deploy" \
		-v "${CI_BUILD_DIR}:/source:ro" \
		hyperionproject/hyperion-ci:$DOCKER_TAG \
		/bin/bash -c "mkdir hyperion && cp -r source/. /hyperion &&
		cd /hyperion && mkdir build && cd build &&
		cmake -DPLATFORM=${PLATFORM} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DDOCKER_PLATFORM=${DOCKER_TAG} ../ || exit 2 &&
		make -j $(nproc) package || exit 3 &&
		cp /hyperion/build/bin/h* /deploy/ 2>/dev/null || : &&
		cp /hyperion/build/Hyperion-* /deploy/ 2>/dev/null || : &&
		cd /hyperion && source /hyperion/test/testrunner.sh || exit 4 &&
		exit 0;
		exit 1 " || { echo "---> Hyperion compilation failed! Abort"; exit 5; }

	# overwrite file owner to current user
	sudo chown -fR $(stat -c "%U:%G" ${CI_BUILD_DIR}/deploy) ${CI_BUILD_DIR}/deploy
fi
