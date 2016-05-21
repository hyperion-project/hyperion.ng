# Define the main-project name
project(hyperiond)

# define the minimum cmake version (as required by cmake)
cmake_minimum_required(VERSION 2.8)

#set(CMAKE_TOOLCHAIN_FILE /opt/raspberrypi/Toolchain-RaspberryPi.cmake)

SET ( DEFAULT_AMLOGIC    OFF )
SET ( DEFAULT_DISPMANX   OFF )
SET ( DEFAULT_FB         OFF )
SET ( DEFAULT_OSX        OFF )
SET ( DEFAULT_X11        OFF )
SET ( DEFAULT_WS2812BPWM OFF )
SET ( DEFAULT_WS281XPWM  OFF )

if (APPLE)
	SET ( DEFAULT_OSX ON )
else ()
	if ( ${PLATFORM} STREQUAL "rpi" )
		SET ( DEFAULT_DISPMANX   ON )
		SET ( DEFAULT_WS2812BPWM ON )
		SET ( DEFAULT_WS281XPWM  ON )
	elseif ( ${PLATFORM} STREQUAL "wetek" )
		SET ( DEFAULT_AMLOGIC    ON )
		SET ( DEFAULT_FB         ON )
	elseif ( ${PLATFORM} STREQUAL "x86" )
		SET ( DEFAULT_X11        ON )
	elseif ( ${PLATFORM} STREQUAL "imx6" )
		SET ( DEFAULT_FB         ON )
	endif()
endif ()

# set the build options
option(ENABLE_AMLOGIC "Enable the AMLOGIC video grabber" ${DEFAULT_AMLOGIC} )
message(STATUS "ENABLE_AMLOGIC = " ${ENABLE_AMLOGIC})

option(ENABLE_DISPMANX "Enable the RPi dispmanx grabber" ${DEFAULT_DISPMANX} )
message(STATUS "ENABLE_DISPMANX = " ${ENABLE_DISPMANX})

option(ENABLE_FB "Enable the framebuffer grabber" ${DEFAULT_FB} )
message(STATUS "ENABLE_FB = " ${ENABLE_FB})

option(ENABLE_OSX "Enable the osx grabber" ${DEFAULT_OSX} )
message(STATUS "ENABLE_OSX = " ${ENABLE_OSX})

option(ENABLE_PROTOBUF "Enable PROTOBUF server" ON)
message(STATUS "ENABLE_PROTOBUF = " ${ENABLE_PROTOBUF})

option(ENABLE_SPIDEV   "Enable the SPIDEV device" ON)
message(STATUS "ENABLE_SPIDEV   = " ${ENABLE_SPIDEV})

option(ENABLE_TINKERFORGE "Enable the TINKERFORGE device" ON)
message(STATUS "ENABLE_TINKERFORGE = " ${ENABLE_TINKERFORGE})

option(ENABLE_V4L2 "Enable the V4L2 grabber" ON)
message(STATUS "ENABLE_V4L2 = " ${ENABLE_V4L2})

option(ENABLE_WS2812BPWM   "Enable the WS2812b-PWM device" ${DEFAULT_WS2812BPWM} )
message(STATUS "ENABLE_WS2812BPWM = " ${ENABLE_WS2812BPWM})

option(ENABLE_WS281XPWM   "Enable the WS281x-PWM device" ${DEFAULT_WS281XPWM} )
message(STATUS "ENABLE_WS281XPWM = " ${ENABLE_WS281XPWM})

option(ENABLE_X11 "Enable the X11 grabber" ${DEFAULT_X11})
message(STATUS "ENABLE_X11 = " ${ENABLE_X11})

option(ENABLE_QT5 "Enable QT5" OFF)
message(STATUS "ENABLE_QT5 = " ${ENABLE_QT5})

option(ENABLE_TESTS "Compile additional test applications" OFF)
message(STATUS "ENABLE_TESTS = " ${ENABLE_TESTS})

if(ENABLE_V4L2 AND NOT ENABLE_PROTOBUF)
	message(FATAL_ERROR "V4L2 grabber requires PROTOBUF. Disable V4L2 or enable PROTOBUF")
endif()

if(ENABLE_FB AND ENABLE_DISPMANX)
	message(FATAL_ERROR "dispmanx grabber and framebuffer grabber cannot be used at the same time")
endif()

if(ENABLE_FB AND ENABLE_OSX)
	message(FATAL_ERROR "osx grabber and framebuffer grabber cannot be used at the same time")
endif()

if(ENABLE_OSX AND ENABLE_DISPMANX)
	message(FATAL_ERROR "dispmanx grabber and osx grabber cannot be used at the same time")
endif()

if (DEFINED INSTALL_PREFIX)
	SET( ENABLE_SYSTEM_INSTALL ON)
	SET( CMAKE_INSTALL_PREFIX "${INSTALL_PREFIX}" )
else()
	SET(ENABLE_SYSTEM_INSTALL OFF)
endif()

SET ( PROTOBUF_INSTALL_BIN_DIR ${CMAKE_BINARY_DIR}/proto )
SET ( PROTOBUF_INSTALL_LIB_DIR ${CMAKE_BINARY_DIR}/proto )

#if(ENABLE_QT5)
# TODO vs ENABLE_QT4?
#endif(ENABLE_QT5)

# Createt the configuration file

# Add project specific cmake modules (find, etc)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(GitVersion)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file("${PROJECT_SOURCE_DIR}/HyperionConfig.h.in" "${PROJECT_BINARY_DIR}/HyperionConfig.h")
include_directories("${PROJECT_BINARY_DIR}")

if(ENABLE_QT5)
	ADD_DEFINITIONS ( -DENABLE_QT5 )
	#find_package(Qt5Widgets)
else()
	# Add specific cmake modules to find qt4 (default version finds first available QT which might not be qt4)
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/qt4)
endif()

# Define the global output path of binaries
SET(LIBRARY_OUTPUT_PATH    ${PROJECT_BINARY_DIR}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(DEPLOY_DIR "${CMAKE_SOURCE_DIR}/deploy/hyperion" )
file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_PATH})
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

# Add the project include directory as additional include path
include_directories(${CMAKE_SOURCE_DIR}/dependencies/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Prefer static linking over dynamic
#set(CMAKE_FIND_LIBRARY_SUFFIXES ".a;.so")

# enable C++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -std=c++11 -Wall")

if(ENABLE_QT5)
#find_package(Qt5Core REQUIRED)
	find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}    ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
#	set(CMAKE_CXX_FLAGS "-fPIC")
else()
	# Configure the use of QT4
	find_package(Qt4 COMPONENTS QtCore QtNetwork QtGui REQUIRED QUIET)
endif()

#add libusb and pthreads
find_package(libusb-1.0 REQUIRED)
find_package(Threads REQUIRED)
if(ENABLE_QT5)
	#include(${QT_USE_FILE})
	add_definitions(${QT_DEFINITIONS})
else()
	include(${QT_USE_FILE})
	add_definitions(${QT_DEFINITIONS})
endif()

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
if (ENABLE_TESTS)
	add_subdirectory(test)
endif ()

# Add the doxygen generation directory
add_subdirectory(doc)

# uninstall target
configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

