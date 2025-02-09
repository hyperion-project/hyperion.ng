#!/bin/bash

# set environment variables if not exists
[ -z "${BUILD_TYPE}" ] && BUILD_TYPE="Debug"
[ -z "${TARGET_ARCH}" ] && TARGET_ARCH="linux/amd64"
[ -z "${ENTRYPOINT}" ] && ENTRYPOINT=""
[ -z "${PLATFORM}" ] && PLATFORM="x11"
[ -z "${CPACK_SYSTEM_PROCESSOR}" ] && CPACK_SYSTEM_PROCESSOR=""

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
	cmake -B build -G Ninja -DPLATFORM=${PLATFORM} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=/usr/local || exit 2
	cmake --build build --target package --parallel $(sysctl -n hw.ncpu) || exit 3
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
	# verification bypass of external dependencies
	# set GitHub Container Registry url
	REGISTRY_URL="ghcr.io/hyperion-project/${DOCKER_IMAGE}"
	# take ownership of deploy dir
	mkdir ${GITHUB_WORKSPACE}/deploy

	# run docker
	docker run --rm --platform=${TARGET_ARCH} ${ENTRYPOINT} \
		-v "${GITHUB_WORKSPACE}/deploy:/deploy" \
		-v "${GITHUB_WORKSPACE}:/source:rw" \
		$REGISTRY_URL:$DOCKER_TAG \
		/bin/bash -c "mkdir -p /source/build && cd /source/build &&
		cmake -G Ninja -DPLATFORM=${PLATFORM} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ${CPACK_SYSTEM_PROCESSOR} .. || exit 2 &&
		cmake --build . --target package -- -j $(nproc) || exit 3 || : &&
		cp /source/build/bin/h* /deploy/ 2>/dev/null || : &&
		cp /source/build/Hyperion-* /deploy/ 2>/dev/null || : &&
		cd /source && source /source/test/testrunner.sh || exit 5 &&
		exit 0;
		exit 1 " || { echo "---> Hyperion compilation failed! Abort"; exit 5; }

	# overwrite file owner to current user
	sudo chown -fR $(stat -c "%U:%G" ${GITHUB_WORKSPACE}/deploy) ${GITHUB_WORKSPACE}/deploy
fi
