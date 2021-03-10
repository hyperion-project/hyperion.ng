#!/bin/bash -e

DOCKER="docker"
# Git repo url of Hyperion
GIT_REPO_URL="https://github.com/hyperion-project/hyperion.ng.git"
# GitHub Container Registry url
REGISTRY_URL="ghcr.io/hyperion-project"
# cmake build type
BUILD_TYPE="Release"
# the docker image at GitHub Container Registry
BUILD_IMAGE="x86_64"
# the docker tag at GitHub Container Registry
BUILD_TAG="buster"
# build packages (.deb .zip ...)
BUILD_PACKAGES=true
# packages string inserted to cmake cmd
PACKAGES=""
#Run build using GitHub code files
BUILD_LOCAL=0
#Build from scratch
BUILD_INCREMENTAL=0
#Verbose output
_VERBOSE=0

# get current path to this script, independent of calling
pushd . > /dev/null
BASE_PATH="${BASH_SOURCE[0]}"
if ([ -h "${BASE_PATH}" ]); then
  while([ -h "${BASE_PATH}" ]); do cd `dirname "${BASE_PATH}"`;
  BASE_PATH=`readlink "${BASE_PATH}"`; done
fi
cd `dirname ${BASE_PATH}` > /dev/null
BASE_PATH=`pwd`;
popd  > /dev/null

BASE_PATH=`pwd`;function log () {
    if [[ $_V -eq 1 ]]; then
        echo "$@"
    fi
}

set +e
${DOCKER} ps >/dev/null 2>&1
if [ $? != 0 ]; then
	DOCKER="sudo docker"
fi
# check if docker is available
if ! ${DOCKER} ps >/dev/null; then
	echo "Error connecting to docker:"
	${DOCKER} ps
	printHelp
	exit 1
fi
set -e

# help print function
function printHelp {
echo "########################################################
## A script to compile Hyperion inside a docker container
## Requires installed Docker: https://www.docker.com/
## Without arguments it will compile Hyperion for Debian Buster (x86_64) and uses Hyperion code from GitHub repository.
## Supports Raspberry Pi (armv6l, armv7l) cross compilation (Debian Stretch/Buster) and native compilation (Raspbian Stretch/Buster)
##
## Homepage: https://www.hyperion-project.org
## Forum: https://forum.hyperion-project.org
########################################################
# These are possible arguments to modify the script behaviour with their default values
#
# docker-compile.sh -h            # Show this help message
# docker-compile.sh -i x86_64     # The docker image, one of x86_64 | armv6l | armv7l | rpi-raspbian
# docker-compile.sh -t stretch    # The docker tag, stretch or buster
# docker-compile.sh -b Release    # cmake Release or Debug build
# docker-compile.sh -p true       # If true, build packages with CPack
# docker-compile.sh -l            # Run build using local code files
# docker-compile.sh -c            # Run incremental build, i.e. do not delete files created during previous build
# More informations to docker tags at: https://github.com/Hyperion-Project/hyperion.docker-ci"
}

function log () {
    if [[ $_VERBOSE -eq 1 ]]; then
        echo "$@"
    fi
}

echo "Compile Hyperion using a Docker container"

while getopts i:t:b:p:lcvh option
do
 case "${option}"
 in
 i) BUILD_IMAGE=${OPTARG};;
 t) BUILD_TAG=${OPTARG};;
 b) BUILD_TYPE=${OPTARG};;
 p) BUILD_PACKAGES=${OPTARG};;
 l) BUILD_LOCAL=1;;
 c) BUILD_INCREMENTAL=1;;
 v) _VERBOSE=1;;    
 h) printHelp; exit 0;;
 esac
done

# determine package creation
if [ ${BUILD_PACKAGES} == "true" ]; then
	PACKAGES="package"
fi

echo "---> Initialize with IMAGE:TAG=${BUILD_IMAGE}:${BUILD_TAG}, BUILD_TYPE=${BUILD_TYPE}, BUILD_PACKAGES=${BUILD_PACKAGES}, BUILD_LOCAL=${BUILD_LOCAL}, BUILD_INCREMENTAL=${BUILD_INCREMENTAL}"

CODE_PATH=${BASE_PATH};
BUILD_DIR="build-${BUILD_IMAGE}-${BUILD_TAG}"
BUILD_PATH="${BASE_PATH}/${BUILD_DIR}"
DEPLOY_DIR="deploy/${BUILD_IMAGE}/${BUILD_TAG}"
DEPLOY_PATH="${BASE_PATH}/${DEPLOY_DIR}"

log "---> BASE_PATH  = ${BASE_PATH}"
log "---> BUILD_DIR  = ${BUILD_DIR}"
log "---> BUILD_PATH = ${BUILD_PATH}"
log "---> DEPLOY_DIR = ${DEPLOY_DIR}"
log "---> DEPLOY_PATH = ${DEPLOY_PATH}"

# cleanup deploy folder, create folder for ownership
sudo rm -fr ${DEPLOY_PATH} >/dev/null 2>&1
mkdir -p ${DEPLOY_PATH} >/dev/null 2>&1

# get Hyperion source, cleanup previous folder
if [ ${BUILD_LOCAL} == 0 ]; then
CODE_PATH="${CODE_PATH}/hyperion/"
echo "---> Downloading Hyperion source code from ${GIT_REPO_URL}"
sudo rm -fr ${CODE_PATH} >/dev/null 2>&1
git clone --recursive --depth 1 -q ${GIT_REPO_URL} ${CODE_PATH} || { echo "---> Failed to download Hyperion source code! Abort"; exit 1; }
fi

#Remove previous build area, if no incremental build
if [ ${BUILD_INCREMENTAL} != 1 ]; then
sudo rm -fr ${BASE_PATH}/${BUILD_PATH} >/dev/null 2>&1
fi
mkdir -p ${BASE_PATH}/${BUILD_PATH} >/dev/null 2>&1

echo "---> Compiling Hyperion from source code at ${CODE_PATH}"

# Steps:
# Update lokal docker image
# Remove container after stop
# Mount deploment path to /deploy
# Mount source dir to /source
# Use target docker image
# execute inside container all commands on bash

echo "---> Startup docker..."
$DOCKER run --rm \
	-v "${DEPLOY_PATH}:/deploy" \
	-v "${CODE_PATH}/:/source:rw" \
	${REGISTRY_URL}/${BUILD_IMAGE}:${BUILD_TAG} \
	/bin/bash -c "mkdir -p /source/${BUILD_DIR} && cd /source/${BUILD_DIR} &&
	cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} .. || exit 2 &&
	make -j $(nproc) ${PACKAGES} || exit 3 &&
	echo '---> Copying packages to host folder: ${DEPLOY_PATH}' &&
	cp -v /source/${BUILD_DIR}/Hyperion-* /deploy 2>/dev/null || : &&
	exit 0;
	exit 1 " || { echo "---> Hyperion compilation failed! Abort"; exit 4; }

# overwrite file owner to current user
sudo chown -fR $(stat -c "%U:%G" ${BASE_PATH}) ${BUILD_PATH}
sudo chown -fR $(stat -c "%U:%G" ${BASE_PATH}) ${DEPLOY_PATH}

if [ ${BUILD_LOCAL} == 1 ]; then
 echo "---> Find compiled binaries in: ${BUILD_PATH}/bin"
fi

if [ ${BUILD_PACKAGES} == "true" ]; then
 echo "---> Find deployment packages in: ${DEPLOY_PATH}"
fi
 echo "---> Script finished"
exit 0
