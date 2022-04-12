#!/bin/bash

# detect CI
if [ "$HOME" != "" ]; then
	# GitHub Actions
	echo "Github Actions detected"
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

echo "Platform: ${PLATFORM}, build type: ${BUILD_TYPE}, CI_NAME: $CI_NAME, docker image: ${DOCKER_IMAGE}, docker type: ${DOCKER_TAG}"

# Build the package on osx or linux
if [[ "$CI_NAME" == 'osx' || "$CI_NAME" == 'darwin' ]]; then
	echo "Compile Hyperion on OSX or Darwin"
	# compile prepare
	mkdir build || exit 1
	cd build
	cmake -DPLATFORM=${PLATFORM} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=/usr/local ../ || exit 2
	make -j $(sysctl -n hw.ncpu) package || exit 3
	cd ${CI_BUILD_DIR} && source /${CI_BUILD_DIR}/test/testrunner.sh || exit 4
	exit 0;
	exit 1 || { echo "---> Hyperion compilation failed! Abort"; exit 5; }
elif [[ $CI_NAME == *"mingw64_nt"* || "$CI_NAME" == 'windows_nt' ]]; then
	echo "Compile Hyperion on Windows"
	# compile prepare
	echo "Number of Cores $NUMBER_OF_PROCESSORS"
	mkdir build || exit 1
	cd build
	cmake -G "Visual Studio 17 2022" -A x64 -DPLATFORM=${PLATFORM} -DCMAKE_BUILD_TYPE="Release" ../ || exit 2
	cmake --build . --target package --config "Release" -- -nologo -v:m -maxcpucount || exit 3
	exit 0;
	exit 1 || { echo "---> Hyperion compilation failed! Abort"; exit 5; }
elif [[ "$CI_NAME" == 'linux' ]]; then
	echo "Compile Hyperion with DOCKER_IMAGE = ${DOCKER_IMAGE}, DOCKER_TAG = ${DOCKER_TAG} and friendly name DOCKER_NAME = ${DOCKER_NAME}"
	# set GitHub Container Registry url
	REGISTRY_URL="ghcr.io/hyperion-project/${DOCKER_IMAGE}"
	# take ownership of deploy dir
	mkdir ${CI_BUILD_DIR}/deploy

	# run docker
	docker run --rm \
		-v "${CI_BUILD_DIR}/deploy:/deploy" \
		-v "${CI_BUILD_DIR}:/source:ro" \
		$REGISTRY_URL:$DOCKER_TAG \
		/bin/bash -c "mkdir hyperion && cp -r source/. /hyperion &&
		cd /hyperion && mkdir build && cd build &&
		cmake -DPLATFORM=${PLATFORM} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ../ || exit 2 &&
		make -j $(nproc) package || exit 3 &&
		cp /hyperion/build/bin/h* /deploy/ 2>/dev/null || : &&
		cp /hyperion/build/Hyperion-* /deploy/ 2>/dev/null || : &&
		cd /hyperion && source /hyperion/test/testrunner.sh || exit 4 &&
		exit 0;
		exit 1 " || { echo "---> Hyperion compilation failed! Abort"; exit 5; }

	# overwrite file owner to current user
	sudo chown -fR $(stat -c "%U:%G" ${CI_BUILD_DIR}/deploy) ${CI_BUILD_DIR}/deploy
fi
