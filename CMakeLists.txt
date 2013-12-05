# Define the main-project name
project(Hyperion)

# define the minimum cmake version (as required by cmake)
cmake_minimum_required(VERSION 2.8)

#set(CMAKE_TOOLCHAIN_FILE /opt/raspberrypi/Toolchain-RaspberryPi.cmake)

# set the build options
option (ENABLE_DISPMANX "Enable the RPi dispmanx grabber" ON)
message(STATUS "ENABLE_DISPMANX = " ${ENABLE_DISPMANX})

# Createt the configuration file
# configure a header file to pass some of the CMake settings
# to the source code
configure_file ("${PROJECT_SOURCE_DIR}/HyperionConfig.h.in" "${PROJECT_BINARY_DIR}/HyperionConfig.h")
include_directories("${PROJECT_BINARY_DIR}")

# Add project specific cmake modules (find, etc)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
# Add specific cmake modules to find qt4 (default version finds first available QT which might not be qt4)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/qt4)

# Define the global output path of binaries
SET(LIBRARY_OUTPUT_PATH    ${PROJECT_BINARY_DIR}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_PATH})
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

# Add the project include directory as additional include path
include_directories(${CMAKE_SOURCE_DIR}/dependencies/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Prefer static linking over dynamic
#set(CMAKE_FIND_LIBRARY_SUFFIXES ".a;.so")

#set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_BUILD_TYPE "Release")

# enable C++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall")

# Configure the use of QT4
find_package(Qt4 COMPONENTS QtCore QtGui QtNetwork REQUIRED QUIET)

# add protocol buffers
find_package(Protobuf REQUIRED)

#add libusb and pthreads
find_package(libusb-1.0 REQUIRED)
find_package(Threads REQUIRED)

include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})
link_directories(${CMAKE_FIND_ROOT_PATH}/lib/arm-linux-gnueabihf)

configure_file(bin/install_hyperion.sh ${LIBRARY_OUTPUT_PATH} @ONLY)
configure_file(config/hyperion.config.json ${LIBRARY_OUTPUT_PATH} @ONLY)
configure_file(config/hyperion_x86.config.json ${LIBRARY_OUTPUT_PATH} @ONLY)

# Add the source/lib directories
add_subdirectory(dependencies)
add_subdirectory(libsrc)
add_subdirectory(src)
add_subdirectory(test)

# Add the doxygen generation directory
add_subdirectory(doc)
