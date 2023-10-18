#!/bin/bash

# set environment variables if not exists
[ -z "${BUILD_TYPE}" ] && BUILD_TYPE="Debug"
[ -z "${TARGET_ARCH}" ] && TARGET_ARCH="linux/amd64"
[ -z "${PLATFORM}" ] && PLATFORM="x11"

# Determine cmake build type; tag builds are Release, else Debug (-dev appends to platform)
if [[ $GITHUB_REF == *"refs/tags"* ]]; then
	BUILD_TYPE=Release
else
	PLATFORM=${PLATFORM}-dev
fi

echo "Compile Hyperion on '${RUNNER_OS}' with build type '${BUILD_TYPE}' and platform '${PLATFORM}'"

# Build the package on MacOS, Windows or Linux
if [[ "$RUNNER_OS" == 'macOS' ]]; then
	mkdir build || exit 1
	cd build
	cmake -DPLATFORM=${PLATFORM} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=/usr/local ../ || exit 2
	make -j $(sysctl -n hw.ncpu) package || exit 3
	cd ${GITHUB_WORKSPACE} && source /${GITHUB_WORKSPACE}/test/testrunner.sh || exit 4
	exit 0;
	exit 1 || { echo "---> Hyperion compilation failed! Abort"; exit 5; }
elif [[ $RUNNER_OS == "Windows" ]]; then
	echo "Number of Cores $NUMBER_OF_PROCESSORS"
	mkdir build || exit 1
	cd build
	cmake -G "Visual Studio 17 2022" -A x64 -DPLATFORM=${PLATFORM} -DCMAKE_BUILD_TYPE="Release" ../ || exit 2
	cmake --build . --target package --config "Release" -- -nologo -v:m -maxcpucount || exit 3
	exit 0;
	exit 1 || { echo "---> Hyperion compilation failed! Abort"; exit 5; }
elif [[ "$RUNNER_OS" == 'Linux' ]]; then
	echo "Docker arguments used: DOCKER_IMAGE=${DOCKER_IMAGE}, DOCKER_TAG=${DOCKER_TAG}, TARGET_ARCH=${TARGET_ARCH}"
	# set GitHub Container Registry url
	REGISTRY_URL="ghcr.io/hyperion-project/${DOCKER_IMAGE}"
	# take ownership of deploy dir
	mkdir ${GITHUB_WORKSPACE}/deploy

	# run docker
	docker run --rm --platform=${TARGET_ARCH} \
		-v "${GITHUB_WORKSPACE}/deploy:/deploy" \
		-v "${GITHUB_WORKSPACE}:/source:ro" \
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
	sudo chown -fR $(stat -c "%U:%G" ${GITHUB_WORKSPACE}/deploy) ${GITHUB_WORKSPACE}/deploy
fi
