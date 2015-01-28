# Define the main-project name
project(Hyperion)

# define the minimum cmake version (as required by cmake)
cmake_minimum_required(VERSION 2.8)

#set(CMAKE_TOOLCHAIN_FILE /opt/raspberrypi/Toolchain-RaspberryPi.cmake)

# set the build options
option(ENABLE_DISPMANX "Enable the RPi dispmanx grabber" ON)
message(STATUS "ENABLE_DISPMANX = " ${ENABLE_DISPMANX})

option(ENABLE_SPIDEV   "Enable the SPIDEV device" ON)
message(STATUS "ENABLE_SPIDEV   = " ${ENABLE_SPIDEV})

option(ENABLE_WS2812BPWM   "Enable the WS2812b-PWM device" OFF)
message(STATUS "ENABLE_WS2812BPWM = " ${ENABLE_WS2812BPWM})

option(ENABLE_V4L2 "Enable the V4L2 grabber" ON)
message(STATUS "ENABLE_V4L2 = " ${ENABLE_V4L2})

option(ENABLE_X11 "Enable the X11 grabber" OFF)
message(STATUS "ENABLE_X11 = " ${ENABLE_X11})

option(ENABLE_TINKERFORGE "Enable the TINKERFORGE device" ON)
message(STATUS "ENABLE_TINKERFORGE = " ${ENABLE_TINKERFORGE})

option(ENABLE_PROTOBUF "Enable PROTOBUF server" ON)
message(STATUS "ENABLE_PROTOBUF = " ${ENABLE_PROTOBUF})

if(ENABLE_V4L2 AND NOT ENABLE_PROTOBUF)
	message(FATAL_ERROR "V4L2 grabber requires PROTOBUF. Disable V4L2 or enable PROTOBUF")
endif(ENABLE_V4L2 AND NOT ENABLE_PROTOBUF)

option(ENABLE_FB "Enable the framebuffer grabber" OFF)
message(STATUS "ENABLE_FB = " ${ENABLE_FB})

option(ENABLE_OSX "Enable the osx grabber" OFF)
message(STATUS "ENABLE_OSX = " ${ENABLE_OSX})

if(ENABLE_FB AND ENABLE_DISPMANX)
	message(FATAL_ERROR "dispmanx grabber and framebuffer grabber cannot be used at the same time")
endif(ENABLE_FB AND ENABLE_DISPMANX)

if(ENABLE_FB AND ENABLE_OSX)
	message(FATAL_ERROR "osx grabber and framebuffer grabber cannot be used at the same time")
endif(ENABLE_FB AND ENABLE_OSX)

if(ENABLE_OSX AND ENABLE_DISPMANX)
	message(FATAL_ERROR "dispmanx grabber and osx grabber cannot be used at the same time")
endif(ENABLE_OSX AND ENABLE_DISPMANX)

# Createt the configuration file
# configure a header file to pass some of the CMake settings
# to the source code
configure_file("${PROJECT_SOURCE_DIR}/HyperionConfig.h.in" "${PROJECT_BINARY_DIR}/HyperionConfig.h")
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

set(CMAKE_BUILD_TYPE "Debug")
#set(CMAKE_BUILD_TYPE "Release")

# enable C++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -std=c++11 -Wall")

# Configure the use of QT4
find_package(Qt4 COMPONENTS QtCore QtGui QtNetwork REQUIRED QUIET)

#add libusb and pthreads
find_package(libusb-1.0 REQUIRED)
find_package(Threads REQUIRED)

if (ENABLE_TINKERFORGE)
	#find_package(libtinkerforge-1.0 REQUIRED)
endif (ENABLE_TINKERFORGE)

include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})
# TODO[TvdZ]: This linking directory should only be added if we are cross compiling
if(NOT APPLE)
	link_directories(${CMAKE_FIND_ROOT_PATH}/lib/arm-linux-gnueabihf)
endif()

if(ENABLE_OSX)
	set(CMAKE_EXE_LINKER_FLAGS "-framework CoreGraphics")
endif()

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
