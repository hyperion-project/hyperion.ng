#!/bin/bash -e

DOCKER="docker"
# Git repo url of Hyperion
GIT_REPO_URL="https://github.com/hyperion-project/hyperion.ng.git"
# cmake build type
BUILD_TYPE="Release"
# the image tag at hyperionproject/hyperion-ci
BUILD_TARGET="amd64"
# build packages (.deb .zip ...)
BUILD_PACKAGES=true
# packages string inserted to cmake cmd
PACKAGES=""

# get current path to this script, independent of calling
pushd . > /dev/null
SCRIPT_PATH="${BASH_SOURCE[0]}"
if ([ -h "${SCRIPT_PATH}" ]); then
  while([ -h "${SCRIPT_PATH}" ]); do cd `dirname "$SCRIPT_PATH"`;
  SCRIPT_PATH=`readlink "${SCRIPT_PATH}"`; done
fi
cd `dirname ${SCRIPT_PATH}` > /dev/null
SCRIPT_PATH=`pwd`;
popd  > /dev/null

set +e
$DOCKER ps >/dev/null 2>&1
if [ $? != 0 ]; then
	DOCKER="sudo docker"
fi
# check if docker is available
if ! $DOCKER ps >/dev/null; then
	echo "Error connecting to docker:"
	$DOCKER ps
	printHelp
	exit 1
fi
set -e

# help print function
function printHelp {
echo "########################################################
## A script to compile Hyperion inside a docker container
## Requires installed Docker: https://www.docker.com/
## Without arguments it will compile Hyperion for Debain Stretch (x64) or higher.
## Supports Raspberry Pi (armv6hf, armv7hf) cross compilation (Debian Stretch/Buster) and native compilation (Raspbian Stretch/Buster)
##
## Homepage: https://www.hyperion-project.org
## Forum: https://forum.hyperion-project.org
########################################################
# These are possible arguments to modify the script behaviour with their default values
#
# docker-compile.sh -h	            # Show this help message
# docker-compile.sh -t amd64        # The docker tag, one of amd64 | i386 | armv6hf | armv7hf | armv6hf-buster | armv7hf-buster | rpi-raspbian-stretch | rpi-raspbian-buster
# docker-compile.sh -b Release      # cmake Release or Debug build
# docker-compile.sh -p true         # If true build packages with CPack
# More informations to docker tags at: https://hub.docker.com/r/hyperionproject/hyperion-ci/"
}

while getopts t:b:p:h option
do
 case "${option}"
 in
 t) BUILD_TARGET=${OPTARG};;
 b) BUILD_TYPE=${OPTARG};;
 p) BUILD_PACKAGES=${OPTARG};;
 h) printHelp; exit 0;;
 esac
done

# determine package creation
if [ $BUILD_PACKAGES == "true" ]; then
	PACKAGES="package"
fi

echo "---> Initialize with BUILD_TARGET=${BUILD_TARGET}, BUILD_TYPE=${BUILD_TYPE}, BUILD_PACKAGES=${BUILD_PACKAGES}"

# cleanup deploy folder, create folder for ownership
sudo rm -fr $SCRIPT_PATH/deploy >/dev/null 2>&1
mkdir $SCRIPT_PATH/deploy >/dev/null 2>&1

# get Hyperion source, cleanup previous folder
echo "---> Downloading Hyperion source code from ${GIT_REPO_URL}"
sudo rm -fr $SCRIPT_PATH/hyperion >/dev/null 2>&1
git clone --recursive --depth 1 -q $GIT_REPO_URL $SCRIPT_PATH/hyperion || { echo "---> Failed to download Hyperion source code! Abort"; exit 1; }

# start compilation
# Remove container after stop
# Mount /deploy to /deploy
# Mount source dir to /source
# Target docker image
# execute inside container all commands on bash
echo "---> Startup docker..."
$DOCKER pull hyperionproject/hyperion-ci:$BUILD_TARGET
$DOCKER run --rm \
	-v "${SCRIPT_PATH}/deploy:/deploy" \
	-v "${SCRIPT_PATH}/hyperion:/source:ro" \
	hyperionproject/hyperion-ci:$BUILD_TARGET \
	/bin/bash -c "mkdir hyperion && cp -r /source/. /hyperion &&
	cd /hyperion && mkdir build && cd build &&
	cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} .. || exit 2 &&
	make -j $(nproc) ${PACKAGES} || exit 3 &&
	echo '---> Copy binaries and packages to host folder: ${SCRIPT_PATH}/deploy' &&
	cp -v /hyperion/build/bin/h* /deploy/ 2>/dev/null || : &&
	cp -v /hyperion/build/Hyperion-* /deploy/ 2>/dev/null || : &&
	exit 0;
	exit 1 " || { echo "---> Hyperion compilation failed! Abort"; exit 4; }

# overwrite file owner to current user
sudo chown -fR $(stat -c "%U:%G" $SCRIPT_PATH/deploy) $SCRIPT_PATH/deploy

echo "---> Script finished, view folder ${SCRIPT_PATH}/deploy for compiled packages and binaries"
exit 0
