
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

#set(CMAKE_BUILD_TYPE "Release")

# enable C++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall")

configure_file(bin/install_hyperion.sh ${LIBRARY_OUTPUT_PATH} @ONLY)
configure_file(config/Hyperion.config.json ${LIBRARY_OUTPUT_PATH} @ONLY)
configure_file(config/Hyperion.schema.json ${LIBRARY_OUTPUT_PATH} @ONLY)

# Add the source/lib directories
add_subdirectory(dependencies)
add_subdirectory(libsrc)
add_subdirectory(src)
