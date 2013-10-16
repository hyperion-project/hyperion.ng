
# define the minimum cmake version (as required by cmake)
cmake_minimum_required(VERSION 2.8)

#set(CMAKE_TOOLCHAIN_FILE /opt/raspberrypi/Toolchain-RaspberryPi.cmake)

# Define the main-project name
project(Hyperion)

# Add project specific cmake modules (find, etc)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Define the global output path of binaries
SET(LIBRARY_OUTPUT_PATH    ${PROJECT_BINARY_DIR}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_PATH})
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

# Add the project include directory as additional include path
include_directories(${CMAKE_SOURCE_DIR}/dependencies/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Prefer static linking over dynamic
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a;.so")

set(CMAKE_BUILD_TYPE "Release")

# enable C++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall")

# Configure the use of QT4
find_package(Qt4 COMPONENTS QtCore QtGui QtNetwork REQUIRED QUIET)

# add protocol buffers
find_package(Protobuf REQUIRED)

#SET(QT_DONT_USE_QTGUI   TRUE)
#SET(QT_USE_QTCONSOLE    TRUE)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})
link_directories(${CMAKE_FIND_ROOT_PATH}/lib/arm-linux-gnueabihf)

configure_file(bin/install_hyperion.sh ${LIBRARY_OUTPUT_PATH} @ONLY)
configure_file(config/hyperion.config.json ${LIBRARY_OUTPUT_PATH} @ONLY)

# Add the source/lib directories
add_subdirectory(dependencies)
add_subdirectory(libsrc)
add_subdirectory(src)
add_subdirectory(test)

# Add the doxygen generation directory
add_subdirectory(doc)
