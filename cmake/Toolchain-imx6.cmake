SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

SET(CROSSROOT $ENV{HOME}/crosscompile)
SET(DEVROOT ${CROSSROOT}/hummingboard)
SET(CUBIXROOT ${DEVROOT}/rootfs)
SET(CUBIXCROSS_DIR ${DEVROOT}/tools)

SET(TOOLROOT ${CUBIXCROSS_DIR}/gcc-linaro-arm-linux-gnueabihf-4.8-2013.10_linux/ )

# specify the cross compiler
SET(CMAKE_C_COMPILER   ${TOOLROOT}/bin/arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER ${TOOLROOT}/bin/arm-linux-gnueabihf-g++)
SET(CUBIX_FLAGS "-march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard")
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf" )

SET(CMAKE_SYSROOT ${CUBIXROOT})
SET(CMAKE_FIND_ROOT_PATH ${CUBIXROOT})

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
#SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGES ONLY)

#SET Flags for linking as dynamic linker config file is not being properly parsed by the toolchain
# see https://solderspot.wordpress.com/2016/02/04/cross-compiling-for-raspberry-pi-part-ii/

SET(FLAGS "${CUBIX_FLAGS} -Wl,-rpath-link,${CUBIXROOT}/opt/vc/lib -Wl,-rpath-link,${CUBIXROOT}/lib/arm-linux-gnueabihf -Wl,-rpath-link,${CUBIXROOT}/usr/lib/arm-linux-gnueabihf -Wl,-rpath-link,${CUBIXROOT}/usr/local/lib")

UNSET(CMAKE_C_FLAGS CACHE)
UNSET(CMAKE_CXX_FLAGS CACHE)

SET(CMAKE_CXX_FLAGS ${FLAGS} CACHE STRING "" FORCE)
SET(CMAKE_C_FLAGS ${FLAGS} CACHE STRING "" FORCE)

link_directories(${CUBIXROOT}/lib/arm-linux-gnueabihf)
