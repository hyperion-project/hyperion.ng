#!/bin/sh

DOCKER="docker"
SUDO="sudo"
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

# Set base path to current directory
BASE_PATH=`pwd`;

printHelp() {
echo "########################################################
## Compile Hyperion in Docker
## Requires Docker: https://www.docker.com/
## Default: ${DISTRIBUTION}:${CODENAME}, ${ARCHITECTURE}, code from GitHub
## For all distributions, their images and architectures supported, see https://github.com/orgs/hyperion-project/packages
# More informations to docker containers available at: https://github.com/Hyperion-Project/hyperion.docker-ci
##
## Homepage: https://www.hyperion-project.org
## Forum: https://hyperion-project.org/forum/
########################################################
# Arguments:
# -h, --help          # Show help
# -n, --name          # Distribution codename, e.g. bookworm, trixie, jammy, noble, resolute; Note: for Fedora it is the version number
# -a, --architecture  # Output architecture, e.g. amd64, arm64, arm/v7, arm/v6
# -b, --type          # Release/Debug build
# -p, --packages      # Build packages with CPack
# --qt5               # Build with Qt5, otherwise build with Qt6
# -f, --platform      # cmake PLATFORM parameter, e.g. x11, amlogic-dev
# -l, --local         # Use local code
# -i, --incremental   # Incremental build
# -v, --verbose       # Verbose output
# -- args             # Additional cmake arguments, e.g. -DHYPERION_LIGHT=ON"
}

log() {
    if [ $_VERBOSE -eq 1 ]; then
        echo "$@"
    fi
}

check_distribution() {
    local image="$1"
    local arch="$2"
    local url="${REGISTRY_URL}/${image}:${CODENAME}"

    log "Check for distribution at: $url for architecture: $arch"

    # Attempt a minimal pull (Docker handles GHCR auth if logged in)
    if docker pull --platform "$arch" --quiet "$url" >/dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

#check sudo availability
set +e
${SUDO} -n true >/dev/null 2>&1
if [ $? -ne 0 ]; then
	SUDO=""
fi
set -e

# Check Docker availability
set +e
${DOCKER} ps >/dev/null 2>&1
if [ $? -ne 0 ]; then
    DOCKER="${SUDO} docker"
fi

echo $DOCKER

if ! ${DOCKER} ps >/dev/null 2>&1; then
    echo "Error connecting to docker:"
    ${DOCKER} ps
    printHelp
    exit 1
fi
set -e

echo "Compile Hyperion using Docker"

# Parse arguments
while [ "$#" -gt 0 ]; do
    case "$1" in
        -a|--architecture)
            ARCHITECTURE=$(echo "$2" | tr '[:upper:]' '[:lower:]')
            shift 2
            ;;
        -n|--name)
            CODENAME=$(echo "$2" | tr '[:upper:]' '[:lower:]')
            shift 2
            ;;
        -b|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -p|--packages)
            BUILD_PACKAGES="$2"
            shift 2
            ;;
        -f|--platform)
            BUILD_PLATFORM="$2"
            shift 2
            ;;
        --qt5)
            BUILD_WITH_QT5=true
            shift
            ;;
        -l|--local)
            BUILD_LOCAL=true
            shift
            ;;
        -i|--incremental)
            BUILD_INCREMENTAL=true
            shift
            ;;
        -v|--verbose)
            _VERBOSE=1
            shift
            ;;
        -h|--help)
            printHelp
            exit 0
            ;;
        --)
            shift
            BUILD_ARGS="$@"
            break
            ;;
        *)
            BUILD_ARGS="$@"
            break
            ;;
    esac
done

# Determine cmake parameters
if [ "${BUILD_PACKAGES}" = "true" ]; then
    PACKAGES="--target package"
fi

if [ -n "${BUILD_PLATFORM}" ]; then
    PLATFORM="-DPLATFORM=${BUILD_PLATFORM}"
fi

PLATFORM_ARCHITECTURE="linux/${ARCHITECTURE}"

QTVERSION="5"
if [ "${BUILD_WITH_QT5}" = "false" ]; then
    QTVERSION="6"
    CODENAME="${CODENAME}-qt6"
fi

echo "---> Evaluate distribution for codename:${CODENAME} on platform architecture ${PLATFORM_ARCHITECTURE}"
DISTRIBUTION="debian"
if ! check_distribution ${DISTRIBUTION} ${PLATFORM_ARCHITECTURE}; then
    DISTRIBUTION="ubuntu"
    if ! check_distribution ${DISTRIBUTION} ${PLATFORM_ARCHITECTURE}; then
        DISTRIBUTION="fedora"
        if ! check_distribution ${DISTRIBUTION} ${PLATFORM_ARCHITECTURE}; then
            echo "No docker image found for ${CODENAME} on ${PLATFORM_ARCHITECTURE}"
            exit 1
        fi
    fi
fi

echo "---> Build with -> Distribution: ${DISTRIBUTION}, Codename: ${CODENAME}, Architecture: ${ARCHITECTURE}, Type: ${BUILD_TYPE}, Platform: ${BUILD_PLATFORM}, QT Version: ${QTVERSION}, Build Packages: ${BUILD_PACKAGES}, Build local: ${BUILD_LOCAL}, Build incremental: ${BUILD_INCREMENTAL}"

CURRENT_ARCHITECTURE=$(uname -m)

# Multiarch detection
if [ "${CURRENT_ARCHITECTURE}" = "aarch64" ]; then
    CURRENT_ARCHITECTURE="arm64"
    USER_ARCHITECTURE=$CURRENT_ARCHITECTURE
    IS_V7L=$(grep v7l /proc/$$/maps | head -n1 | wc -l)
    if [ $IS_V7L -ne 0 ]; then
        USER_ARCHITECTURE="arm/v7"
    else
       IS_V6L=$(grep v6l /proc/$$/maps | head -n1 | wc -l)
       if [ $IS_V6L -ne 0 ]; then
           USER_ARCHITECTURE="arm/v6"
       fi
    fi
    if [ "$CURRENT_ARCHITECTURE" != "$USER_ARCHITECTURE" ]; then
        log "Identified user space current architecture: $USER_ARCHITECTURE"
        CURRENT_ARCHITECTURE=$USER_ARCHITECTURE
    fi
else
    CURRENT_ARCHITECTURE=$(echo "${CURRENT_ARCHITECTURE}" | sed 's/x86_/amd/g')
fi

log "Identified kernel current architecture: $CURRENT_ARCHITECTURE"

if [ "$ARCHITECTURE" != "$CURRENT_ARCHITECTURE" ]; then
    echo "---> Build is not for the same architecture, enable emulation for ${PLATFORM_ARCHITECTURE}"
    ENTRYPOINT_OPTION="--entrypoint /usr/bin/qemu-static"
    if [ "$CURRENT_ARCHITECTURE" != "amd64" ]; then
        echo "---> Emulation builds can only run on linux/amd64, current: ${CURRENT_ARCHITECTURE}"
        exit 1
    fi
else
    log "Build natively for platform architecture: ${PLATFORM_ARCHITECTURE}"
    ENTRYPOINT_OPTION="--entrypoint="
fi

log "---> BASE_PATH  = ${BASE_PATH}"
CODE_PATH=${BASE_PATH}

# Clone source if not local
if [ "${BUILD_LOCAL}" = "false" ]; then
    CODE_PATH="${CODE_PATH}/hyperion/"
    echo "---> Downloading Hyperion source code from ${GIT_REPO_URL}"
    ${SUDO} rm -fr "${CODE_PATH}" >/dev/null 2>&1
    git clone --recursive --depth 1 -q ${GIT_REPO_URL} ${CODE_PATH} || { echo "---> Failed to download Hyperion source code! Abort"; exit 1; }
fi

log "---> CODE_PATH  = ${CODE_PATH}"
ARCHITECTURE_PATH=$(echo "${ARCHITECTURE}" | sed 's,/,_,g')
BUILD_DIR="build-${CODENAME}-${ARCHITECTURE_PATH}"
BUILD_PATH="${CODE_PATH}/${BUILD_DIR}"
DEPLOY_DIR="deploy/${CODENAME}/${ARCHITECTURE}"
DEPLOY_PATH="${CODE_PATH}/${DEPLOY_DIR}"

log "---> BUILD_DIR  = ${BUILD_DIR}"
log "---> BUILD_PATH = ${BUILD_PATH}"
log "---> DEPLOY_DIR = ${DEPLOY_DIR}"
log "---> DEPLOY_PATH = ${DEPLOY_PATH}"

# Cleanup deploy folder
${SUDO} rm -fr "${DEPLOY_PATH}" >/dev/null 2>&1
${SUDO} mkdir -p "${DEPLOY_PATH}" >/dev/null 2>&1

# Cleanup build folder if not incremental
if [ "${BUILD_INCREMENTAL}" != "true" ]; then
    ${SUDO} rm -fr "${BUILD_PATH}" >/dev/null 2>&1
    CMAKE_CMD="cmake -G Ninja -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ${PLATFORM} ${BUILD_ARGS} .. &&"
else
    echo "---> Incremental build, keep existing build folder: ${BUILD_PATH}"
    CMAKE_CMD="true"
fi
${SUDO} mkdir -p "${BUILD_PATH}" >/dev/null 2>&1

NPROC=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || echo 1)
echo "---> Compiling Hyperion from source code at ${CODE_PATH} using ${NPROC} CPU cores"

$DOCKER run --rm --platform=${PLATFORM_ARCHITECTURE} \
    ${ENTRYPOINT_OPTION} \
    -v "${DEPLOY_PATH}:/deploy" \
    -v "${CODE_PATH}/:/source:rw" \
    -e LANG="C.UTF-8" \
    -e LC_ALL="C.UTF-8" \
    "${REGISTRY_URL}/${DISTRIBUTION}:${CODENAME}" \
	/bin/bash -c "mkdir -p /source/${BUILD_DIR} && cd /source/${BUILD_DIR} &&
		${CMAKE_CMD} || exit 2 &&
		cmake --build . ${PACKAGES} -- -j ${NPROC} || exit 3 || : &&
		exit 0;
		exit 1 " \
	|| { echo "---> Hyperion compilation failed! Abort"; exit 4; }

DOCKERRC=$?

# Set ownership using POSIX-safe method
OWNER=$(id -u):$(id -g)
${SUDO} chown -fR "$OWNER" "${BUILD_PATH}"

if [ ${DOCKERRC} -eq 0 ]; then
    if [ "${BUILD_LOCAL}" = "true" ]; then
        echo "---> Find compiled binaries in: ${BUILD_PATH}/bin"
    fi
    if [ "${BUILD_PACKAGES}" = "true" ]; then
        echo "---> Copying packages to host folder: ${DEPLOY_PATH}"
        ${SUDO} cp ${BUILD_PATH}/Hyperion-* "${DEPLOY_PATH}" 2>/dev/null
        echo "---> Find deployment packages in: ${DEPLOY_PATH}"
        ${SUDO} chown -fR "$OWNER" "${DEPLOY_PATH}"
    fi
fi

echo "---> Script finished [${DOCKERRC}]"
exit ${DOCKERRC}