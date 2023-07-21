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
BUILD_TAG="bullseye"
# build packages (.deb .zip ...)
BUILD_PACKAGES=true
# packages string inserted to cmake cmd
PACKAGES=""
# platform string inserted to cmake cmd
BUILD_PLATFORM=""
#Run build using GitHub code files
BUILD_LOCAL=0
#Build from scratch
BUILD_INCREMENTAL=0
#Verbose output
_VERBOSE=0
#Additional args
BUILD_ARGS=""

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
## Without arguments it will compile Hyperion for Debian Bullseye (x86_64) and uses Hyperion code from GitHub repository.
## For all images and tags currently available, see https://github.com/orgs/hyperion-project/packages
##
## Homepage: https://www.hyperion-project.org
## Forum: https://hyperion-project.org/forum/
########################################################
# These are possible arguments to modify the script behaviour with their default values
#
# docker-compile.sh -h, --help          # Show this help message
# docker-compile.sh -i, --image         # The docker image, e.g., x86_64, armv6l, armv7l, aarch64
# docker-compile.sh -t, --tag           # The docker tag, e.g., buster, bullseye, bookworm
# docker-compile.sh -b, --type          # Release or Debug build
# docker-compile.sh -p, --packages      # If true, build packages with CPack
# docker-compile.sh -l, --local         # Run build using local code files
# docker-compile.sh -c, --incremental   # Run incremental build, i.e. do not delete files created during previous build
# docker-compile.sh -f, --platform      # cmake PLATFORM parameter, e.g. x11, amlogic-dev
# docker-compile.sh -v, --verbose       # Run the script in verbose mode
# docker-compile.sh     -- args         # Additonal cmake arguments, e.g., -DHYPERION_LIGHT=ON
# More informations to docker tags at: https://github.com/Hyperion-Project/hyperion.docker-ci"
}

function log () {
    if [[ $_VERBOSE -eq 1 ]]; then
        echo "$@"
    fi
}

echo "Compile Hyperion using a Docker container"
options=$(getopt -l "image:,tag:,type:,packages:,platform:,local,incremental,verbose,help" -o "i:t:b:p:f:lcvh" -a -- "$@")

eval set -- "$options"
while true
do
    case $1 in
        -i|--image) 
            shift
            BUILD_IMAGE=$1
            ;;
        -t|--tag) 
            shift
            BUILD_TAG=$1
            ;;
        -b|--type) 
            shift
            BUILD_TYPE=$1
            ;;
        -p|--packages) 
            shift
            BUILD_PACKAGES=$1
            ;;
        -f|--platform) 
            shift
            BUILD_PLATFORM=$1
            ;;
        -l|--local) 
            BUILD_LOCAL=1
            ;;
        -c|--incremental) 
            BUILD_INCREMENTAL=1
            ;;
        -v|--verbose) 
            _VERBOSE=1
            ;;
        -h|--help) 
            printHelp
            exit 0
            ;;
        --)
            shift        
            break;;
    esac
    shift
done

BUILD_ARGS=$@

# determine package creation
if [ ${BUILD_PACKAGES} == "true" ]; then
	PACKAGES="package"
fi

# determine platform cmake parameter
if [[ ! -z ${BUILD_PLATFORM} ]]; then
	PLATFORM="-DPLATFORM=${BUILD_PLATFORM}"
fi

echo "---> Initialize with IMAGE:TAG=${BUILD_IMAGE}:${BUILD_TAG}, BUILD_TYPE=${BUILD_TYPE}, BUILD_PACKAGES=${BUILD_PACKAGES}, PLATFORM=${BUILD_PLATFORM}, BUILD_LOCAL=${BUILD_LOCAL}, BUILD_INCREMENTAL=${BUILD_INCREMENTAL}"

log "---> BASE_PATH  = ${BASE_PATH}"
CODE_PATH=${BASE_PATH};

# get Hyperion source, cleanup previous folder
if [ ${BUILD_LOCAL} == 0 ]; then
CODE_PATH="${CODE_PATH}/hyperion/"

echo "---> Downloading Hyperion source code from ${GIT_REPO_URL}"
sudo rm -fr ${CODE_PATH} >/dev/null 2>&1
git clone --recursive --depth 1 -q ${GIT_REPO_URL} ${CODE_PATH} || { echo "---> Failed to download Hyperion source code! Abort"; exit 1; }
fi
log "---> CODE_PATH  = ${CODE_PATH}"

BUILD_DIR="build-${BUILD_IMAGE}-${BUILD_TAG}"
BUILD_PATH="${CODE_PATH}/${BUILD_DIR}"
DEPLOY_DIR="deploy/${BUILD_IMAGE}/${BUILD_TAG}"
DEPLOY_PATH="${CODE_PATH}/${DEPLOY_DIR}"

log "---> BUILD_DIR  = ${BUILD_DIR}"
log "---> BUILD_PATH = ${BUILD_PATH}"
log "---> DEPLOY_DIR = ${DEPLOY_DIR}"
log "---> DEPLOY_PATH = ${DEPLOY_PATH}"

# cleanup deploy folder, create folder for ownership
sudo rm -fr "${DEPLOY_PATH}" >/dev/null 2>&1
mkdir -p "${DEPLOY_PATH}" >/dev/null 2>&1

#Remove previous build area, if no incremental build
if [ ${BUILD_INCREMENTAL} != 1 ]; then
sudo rm -fr "${BUILD_PATH}" >/dev/null 2>&1
fi
mkdir -p "${BUILD_PATH}" >/dev/null 2>&1

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
	cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ${PLATFORM} ${BUILD_ARGS} .. || exit 2 &&
	make -j $(nproc) ${PACKAGES} || exit 3 || : &&
	exit 0;
	exit 1 " || { echo "---> Hyperion compilation failed! Abort"; exit 4; }

DOCKERRC=${?}

# overwrite file owner to current user
sudo chown -fR $(stat -c "%U:%G" ${BASE_PATH}) ${BUILD_PATH}

if [ ${DOCKERRC} == 0 ]; then
	if [ ${BUILD_LOCAL} == 1 ]; then
	 echo "---> Find compiled binaries in: ${BUILD_PATH}/bin"
	fi

	if [ ${BUILD_PACKAGES} == "true" ]; then
	 echo "---> Copying packages to host folder: ${DEPLOY_PATH}" &&
	 cp -v ${BUILD_PATH}/Hyperion-* ${DEPLOY_PATH} 2>/dev/null
	 echo "---> Find deployment packages in: ${DEPLOY_PATH}"
	 sudo chown -fR $(stat -c "%U:%G" ${BASE_PATH}) ${DEPLOY_PATH}
	fi
fi
echo "---> Script finished [${DOCKERRC}]"
exit ${DOCKERRC}
