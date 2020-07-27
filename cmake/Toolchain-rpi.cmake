SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

SET(CROSSROOT $ENV{HOME}/crosscompile)
SET(DEVROOT ${CROSSROOT}/raspberrypi)
SET(PIROOT ${DEVROOT}/rootfs)
SET(PITOOLCHAIN ${DEVROOT}/tools)

SET(TOOLROOT ${PITOOLCHAIN}/gcc-linaro-7.4.1-2019.02-x86_64_arm-linux-gnueabihf )
SET(QT_BIN_PATH ${CROSSROOT}/Qt5/5.7/gcc_64/bin)

# specify the cross compiler
SET(CMAKE_C_COMPILER   ${TOOLROOT}/bin/arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER ${TOOLROOT}/bin/arm-linux-gnueabihf-g++)
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf" )

SET(CMAKE_SYSROOT ${PIROOT})
SET(CMAKE_FIND_ROOT_PATH ${PIROOT})

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
#SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGES ONLY)


#SET Flags for linking as dynamic linker config file is not being properly parsed by the toolchain
# see https://solderspot.wordpress.com/2016/02/04/cross-compiling-for-raspberry-pi-part-ii/

SET(FLAGS "-Wl,-rpath-link,${PIROOT}/opt/vc/lib -Wl,-rpath-link,${PIROOT}/lib/arm-linux-gnueabihf -Wl,-rpath-link,${PIROOT}/usr/lib/arm-linux-gnueabihf -Wl,-rpath-link,${PIROOT}/usr/local/lib")

UNSET(CMAKE_C_FLAGS CACHE)
UNSET(CMAKE_CXX_FLAGS CACHE)

SET(CMAKE_CXX_FLAGS ${FLAGS} CACHE STRING "" FORCE)
SET(CMAKE_C_FLAGS ${FLAGS} CACHE STRING "" FORCE)

link_directories(${PIROOT}/lib/arm-linux-gnueabihf)

