#!/bin/bash

DOCKER="docker"
# Git repo url of Hyperion
GIT_REPO_URL="https://github.com/hyperion-project/hyperion.ng.git"
# GitHub Container Registry url
REGISTRY_URL="ghcr.io/hyperion-project"
# cmake build type
BUILD_TYPE="Release"
DISTRIBUTION="debian"
CODENAME="bullseye"
ARCHITECTURE="amd64"
# build packages (.deb .zip ...)
BUILD_PACKAGES=true
# packages string inserted to cmake cmd
PACKAGES=""
# platform string inserted to cmake cmd
BUILD_PLATFORM=""
#Run build with Qt6 or Qt5
BUILD_WITH_QT5=false
#Run build using GitHub code files
BUILD_LOCAL=false
#Build from scratch
BUILD_INCREMENTAL=false
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

BASE_PATH=`pwd`;

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
## Without arguments it will compile Hyperion for ${DISTRIBUTION}:${CODENAME}, ${ARCHITECTURE} architecture and uses Hyperion code from GitHub repository.
## For all images and tags currently available, see https://github.com/orgs/hyperion-project/packages
##
## Homepage: https://www.hyperion-project.org
## Forum: https://hyperion-project.org/forum/
########################################################
# These are possible arguments to modify the script behaviour with their default values
#
# docker-compile.sh -h, --help          # Show this help message
# docker-compile.sh -n, --name          # The distribution's codename, e.g., buster, bullseye, bookworm, jammy, trixie, lunar, mantic; Note: for Fedora it is the version number
# docker-compile.sh -a, --architecture  # The output architecture, e.g., amd64, arm64, arm/v7
# docker-compile.sh -b, --type          # Release or Debug build
# docker-compile.sh -p, --packages      # If true, build packages with CPack
# docker-compile.sh     --qt5           # Build with Qt5, otherwise build with Qt6
# docker-compile.sh -f, --platform      # cmake PLATFORM parameter, e.g. x11, amlogic-dev
# docker-compile.sh -l, --local         # Run build using local code files
# docker-compile.sh -c, --incremental   # Run incremental build, i.e. do not delete files created during previous build
# docker-compile.sh -v, --verbose       # Run the script in verbose mode
# docker-compile.sh     -- args         # Additonal cmake arguments, e.g., -DHYPERION_LIGHT=ON
# More informations to docker containers available at: https://github.com/Hyperion-Project/hyperion.docker-ci"
}

function log () {
    if [[ $_VERBOSE -eq 1 ]]; then
        echo "$@"
    fi
}

function check_distribution () {
	url=${REGISTRY_URL}/$1:${CODENAME}

	log "Check for distribution at: $url"
	if $($DOCKER buildx imagetools inspect "$url" 2>&1 | grep -q $2) ; then
		rc=0
	else
		rc=1
	fi
	return $rc
}

echo "Compile Hyperion using a Docker container"
options=$(getopt -l "architecture:,name:,type:,packages:,platform:,qt5,local,incremental,verbose,help" -o "a:n:b:p:f:lcvh" -a -- "$@")

eval set -- "$options"
while true
do
    case $1 in
        -a|--architecture)
            shift
            ARCHITECTURE=`echo $1 | tr '[:upper:]' '[:lower:]'`
            ;;
        -n|--name)
            shift
            CODENAME=`echo $1 | tr '[:upper:]' '[:lower:]'`
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
        --qt5)
            BUILD_WITH_QT5=true
            ;;
        -l|--local)
            BUILD_LOCAL=true
            ;;
        -i|--incremental)
            BUILD_INCREMENTAL=true
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
	PACKAGES="--target package"
fi

# determine platform cmake parameter
if [[ ! -z ${BUILD_PLATFORM} ]]; then
	PLATFORM="-DPLATFORM=${BUILD_PLATFORM}"
fi

PLATFORM_ARCHITECTURE="linux/"${ARCHITECTURE}

QTVERSION="5"
if [ ${BUILD_WITH_QT5} == false ]; then
	QTVERSION="6"
	CODENAME="${CODENAME}-qt6"
fi

echo "---> Evaluate distribution for codename:${CODENAME} on platform architecture ${PLATFORM_ARCHITECTURE}"
DISTRIBUTION="debian"
if ! check_distribution ${DISTRIBUTION} ${PLATFORM_ARCHITECTURE} ; then
	DISTRIBUTION="ubuntu"
	if ! check_distribution ${DISTRIBUTION} ${PLATFORM_ARCHITECTURE} ; then
		DISTRIBUTION="fedora"
		if ! check_distribution ${DISTRIBUTION} ${PLATFORM_ARCHITECTURE} ; then
			echo "No docker image found for a distribution with codename: ${CODENAME} to be build on platform architecture ${PLATFORM_ARCHITECTURE}"
			exit 1
		fi
	fi
fi

echo "---> Build with -> Distribution: ${DISTRIBUTION}, Codename: ${CODENAME}, Architecture: ${ARCHITECTURE}, Type: ${BUILD_TYPE}, Platform: ${BUILD_PLATFORM}, QT Version: ${QTVERSION}, Build Packages: ${BUILD_PACKAGES}, Build local: ${BUILD_LOCAL}, Build incremental: ${BUILD_INCREMENTAL}"

# Determine the current architecture
CURRENT_ARCHITECTURE=`uname -m`

#Test if multiarchitecture setup, i.e. user-space is 32bit
if [ ${CURRENT_ARCHITECTURE} == "aarch64" ]; then
	CURRENT_ARCHITECTURE="arm64"
	USER_ARCHITECTURE=$CURRENT_ARCHITECTURE
	IS_V7L=`cat /proc/$$/maps |grep -m1 -c v7l`
	if [ $IS_V7L -ne 0 ]; then
		USER_ARCHITECTURE="arm/v7"
	else
	   IS_V6L=`cat /proc/$$/maps |grep -m1 -c v6l`
	   if [ $IS_V6L -ne 0 ]; then
		   USER_ARCHITECTURE="arm/v6"
	   fi
	fi
    if [ $CURRENT_ARCHITECTURE != $USER_ARCHITECTURE ]; then
        log "Identified user space current architecture: $USER_ARCHITECTURE"
        CURRENT_ARCHITECTURE=$USER_ARCHITECTURE
    fi
else
	CURRENT_ARCHITECTURE=${CURRENT_ARCHITECTURE//x86_/amd}
fi

log "Identified kernel current architecture: $CURRENT_ARCHITECTURE"
if [ $ARCHITECTURE != $CURRENT_ARCHITECTURE ]; then
	echo "---> Build is not for the same architecturem, enable emulation for ${PLATFORM_ARCHITECTURE}"
	ENTRYPOINT_OPTION="--entrypoint /usr/bin/qemu-static"

	if [ $CURRENT_ARCHITECTURE != "amd64" ]; then
		echo "---> Emulation builds can only be executed on linux/amd64, linux/x86_64 platforms, current architecture is ${CURRENT_ARCHITECTURE}"
		exit 1
	fi
else
	log "Build natively for platform architecture: ${PLATFORM_ARCHITECTURE}"
	ENTRYPOINT_OPTION="--entrypoint="""
fi

log "---> BASE_PATH  = ${BASE_PATH}"
CODE_PATH=${BASE_PATH};

# get Hyperion source, cleanup previous folder
if [ ${BUILD_LOCAL} == false ]; then
CODE_PATH="${CODE_PATH}/hyperion/"

echo "---> Downloading Hyperion source code from ${GIT_REPO_URL}"
sudo rm -fr ${CODE_PATH} >/dev/null 2>&1
git clone --recursive --depth 1 -q ${GIT_REPO_URL} ${CODE_PATH} || { echo "---> Failed to download Hyperion source code! Abort"; exit 1; }
fi
log "---> CODE_PATH  = ${CODE_PATH}"

ARCHITECTURE_PATH=${ARCHITECTURE//\//_}

BUILD_DIR="build-${CODENAME}-${ARCHITECTURE_PATH}"
BUILD_PATH="${CODE_PATH}/${BUILD_DIR}"
DEPLOY_DIR="deploy/${CODENAME}/${ARCHITECTURE}"
DEPLOY_PATH="${CODE_PATH}/${DEPLOY_DIR}"

log "---> BUILD_DIR  = ${BUILD_DIR}"
log "---> BUILD_PATH = ${BUILD_PATH}"
log "---> DEPLOY_DIR = ${DEPLOY_DIR}"
log "---> DEPLOY_PATH = ${DEPLOY_PATH}"

# cleanup deploy folder, create folder for ownership
sudo rm -fr "${DEPLOY_PATH}" >/dev/null 2>&1
sudo mkdir -p "${DEPLOY_PATH}" >/dev/null 2>&1

#Remove previous build area, if no incremental build
if [ ${BUILD_INCREMENTAL} != true ]; then
sudo rm -fr "${BUILD_PATH}" >/dev/null 2>&1
fi
sudo mkdir -p "${BUILD_PATH}" >/dev/null 2>&1

echo "---> Compiling Hyperion from source code at ${CODE_PATH}"

# Steps:
# Update lokal docker image
# Remove container after stop
# Mount deploment path to /deploy
# Mount source dir to /source
# Use target docker image
# execute inside container all commands on bash

echo "---> Startup docker..."
$DOCKER run --rm --platform=${PLATFORM_ARCHITECTURE} \
	${ENTRYPOINT_OPTION} \
	-v "${DEPLOY_PATH}:/deploy" \
	-v "${CODE_PATH}/:/source:rw" \
	-e LANG="C.UTF-8" \
	-e LC_ALL="C.UTF-8" \
	"${REGISTRY_URL}/${DISTRIBUTION}:${CODENAME}" \
	/bin/bash -c "mkdir -p /source/${BUILD_DIR} && cd /source/${BUILD_DIR} &&
	cmake -G Ninja -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ${PLATFORM} ${BUILD_ARGS} .. || exit 2 &&
	cmake --build . ${PACKAGES} -- -j $(nproc) || exit 3 || : &&
	exit 0;
	exit 1 " || { echo "---> Hyperion compilation failed! Abort"; exit 4; }

DOCKERRC=${?}

# overwrite file owner to current user
sudo chown -fR $(stat -c "%U:%G" ${BASE_PATH}) ${BUILD_PATH}

if [ ${DOCKERRC} == 0 ]; then
	if [ ${BUILD_LOCAL} == true ]; then
	 echo "---> Find compiled binaries in: ${BUILD_PATH}/bin"
	fi

	if [ ${BUILD_PACKAGES} == "true" ]; then
	 echo "---> Copying packages to host folder: ${DEPLOY_PATH}" &&
	 sudo cp -v ${BUILD_PATH}/Hyperion-* ${DEPLOY_PATH} 2>/dev/null
	 echo "---> Find deployment packages in: ${DEPLOY_PATH}"
	 sudo chown -fR $(stat -c "%U:%G" ${BASE_PATH}) ${DEPLOY_PATH}
	fi
fi
echo "---> Script finished [${DOCKERRC}]"
exit ${DOCKERRC}
