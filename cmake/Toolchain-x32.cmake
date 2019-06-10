# toolchain file for building a 32bit version on a 64bit host

# Add additional 32-bit libraries
# sudo apt-get install g++-multilib libc6-dev-i386

#TO-DO
# Install QT5 32bit

SET(CROSSROOT $ENV{HOME}/crosscompile)
SET(QT_BIN_PATH ${CROSSROOT}/x86_32-linux-gnu/qt5/bin)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR "i686")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32" CACHE STRING "c++ flags")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -m32" CACHE STRING "c flags")




