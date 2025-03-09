#=============================================================================
# Minimum required CMake version is 3.5.0
#=============================================================================

cmake_minimum_required(VERSION 3.5.0)
message(STATUS "CMake Version: ${CMAKE_VERSION}")

#=============================================================================
# CMake policies
#=============================================================================

# Disable passing /W3 by default on MSVC
if(POLICY CMP0092)
	cmake_policy(SET CMP0092 NEW)
endif()

#=============================================================================
# Our main project :)
#=============================================================================

project(hyperion)

#=============================================================================
# Set the required Cxx standard
#=============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#=============================================================================
# Instruct CMake to run moc automatically when needed and auto prepare .qrc files
#=============================================================================

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

#=============================================================================
# Append our cmake module path (for include, find, etc.)
# Include the helper macros/functions
#=============================================================================

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(HelperMacros)
include(HelperFunctions)

#=============================================================================
# Use ccache as compiler launcher
#=============================================================================

# Configure CCache if available
find_program(CACHE_PROG ccache)
if(CACHE_PROG)
message(STATUS "Building with ccache: ${CACHE_PROG}")
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CACHE_PROG})
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CACHE_PROG})
endif()

#=============================================================================
# Set output directories and add the project/dependencies include directory
#=============================================================================

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(MSVC)
	foreach(CONFIG_TYPE "RELEASE" "DEBUG" "RELWITHDEBINFO" "MINSIZEREL")
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
		set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
		set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
	endforeach()
endif()

include_directories(${CMAKE_SOURCE_DIR}/dependencies/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

#=============================================================================
# Parse semantic version of version file and write version to config
# Determine branch name and build id
#=============================================================================

file (STRINGS ".version" HYPERION_VERSION)
SetVersionNumber(HYPERION ${HYPERION_VERSION})
set(DEFAULT_JSON_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/settings/hyperion.settings.json.default)
file(READ ${DEFAULT_JSON_CONFIG_FILE} DEFAULT_JSON_CONFIG_VAR)
string(REPLACE "configVersionValue" ${HYPERION_VERSION} DEFAULT_JSON_CONFIG_VAR "${DEFAULT_JSON_CONFIG_VAR}")
file(WRITE ${CMAKE_BINARY_DIR}/settings/hyperion.settings.json.default "${DEFAULT_JSON_CONFIG_VAR}")

include(GitVersion)
message(STATUS "Current Version: ${HYPERION_VERSION}")
message(STATUS " - Build: ${HYPERION_BUILD_ID}")

#=============================================================================
# Set build variables
#=============================================================================

# Grabber
set(DEFAULT_AMLOGIC                     OFF)
set(DEFAULT_DISPMANX                    OFF)
set(DEFAULT_DX                          OFF)
set(DEFAULT_DDA                         OFF)
set(DEFAULT_MF                          OFF)
set(DEFAULT_OSX                         OFF)
set(DEFAULT_QT                          ON )
set(DEFAULT_V4L2                        OFF)
set(DEFAULT_AUDIO                       ON )
set(DEFAULT_X11                         OFF)
set(DEFAULT_XCB                         OFF)

# Input
set(DEFAULT_BOBLIGHT_SERVER             ON )
set(DEFAULT_CEC                         OFF)
set(DEFAULT_FLATBUF_SERVER              ON )
set(DEFAULT_PROTOBUF_SERVER             ON )

# Output
set(DEFAULT_FORWARDER                   ON )
set(DEFAULT_FLATBUF_CONNECT             ON )

# LED-Devices
set(DEFAULT_DEV_NETWORK                 ON )
set(DEFAULT_DEV_SERIAL                  ON )
set(DEFAULT_DEV_SPI                     OFF)
set(DEFAULT_DEV_TINKERFORGE             OFF)
set(DEFAULT_DEV_USB_HID                 OFF)
set(DEFAULT_DEV_WS281XPWM               OFF)
set(DEFAULT_DEV_FTDI                    ON )

# Services
set(DEFAULT_EFFECTENGINE                ON )
set(DEFAULT_EXPERIMENTAL                OFF)
set(DEFAULT_MDNS                        ON )
set(DEFAULT_REMOTE_CTL                  ON )

# Build
set(DEFAULT_JSONCHECKS                  ON )
set(DEFAULT_DEPLOY_DEPENDENCIES         ON )
set(DEFAULT_USE_SYSTEM_FLATBUFFERS_LIBS OFF)
set(DEFAULT_USE_SYSTEM_PROTO_LIBS       OFF)
set(DEFAULT_USE_SYSTEM_MBEDTLS_LIBS     OFF)
set(DEFAULT_USE_SYSTEM_QMDNS_LIBS       OFF)
set(DEFAULT_TESTS                       OFF)

# Build Hyperion with a reduced set of functionality, overwrites other default values
set(DEFAULT_HYPERION_LIGHT              OFF)

if(${CMAKE_SYSTEM} MATCHES "Linux")
	set(DEFAULT_FB              ON)
	set(DEFAULT_V4L2            ON)
	set(DEFAULT_DEV_SPI         ON)
	set(DEFAULT_DEV_TINKERFORGE ON)
	set(DEFAULT_DEV_USB_HID     ON)
	set(DEFAULT_CEC             ON)
elseif (WIN32)
	set(DEFAULT_DX              ON )
	set(DEFAULT_DDA             ON )
	set(DEFAULT_MF              ON )
	set(DEFAULT_DEV_FTDI        OFF)
else()
	set(DEFAULT_FB              OFF)
	set(DEFAULT_V4L2            OFF)
	set(DEFAULT_DEV_SPI         OFF)
	set(DEFAULT_DEV_TINKERFORGE OFF)
	set(DEFAULT_DEV_USB_HID     OFF)
	set(DEFAULT_CEC             OFF)
endif()

#=============================================================================
# Determine/Evaluate the current platform
#=============================================================================

if(NOT DEFINED PLATFORM)
	set(PLATFORM)
	DeterminePlatform(PLATFORM)
	if(NOT PLATFORM)
		message(FATAL_ERROR "PLATFORM is not defined and could not be evaluated. Set -DPLATFORM=<rpi|amlogic|amlogic-dev|amlogic64|amlogic64-dev|x11|x11-dev|osx>")
	endif()
else()
	message(STATUS "PLATFORM: ${PLATFORM}")
endif()

if("${PLATFORM}" MATCHES "osx")
	# specify the min version of the target platform (only GitHub Actions)
	if(DEFINED ENV{GITHUB_WORKSPACE})
		set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
	endif()

	# add specific prefix paths
	FIRSTSUBDIR(SUBDIRPY "/usr/local/opt/python3/Frameworks/Python.framework/Versions")
	set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${SUBDIRPY})

	include_directories("/opt/X11/include/")
	set(DEFAULT_OSX         ON )
	set(DEFAULT_AUDIO       OFF)
	set(DEFAULT_DEV_USB_HID ON )

elseif ("${PLATFORM}" MATCHES "rpi")
	set(DEFAULT_DISPMANX   ON)
	set(DEFAULT_DEV_WS281XPWM  ON)
elseif ("${PLATFORM}" MATCHES "^amlogic")
	set(DEFAULT_AMLOGIC     ON)
    if("${PLATFORM}" MATCHES "-dev$")
		set(DEFAULT_AMLOGIC     ON)
		set(DEFAULT_DISPMANX    OFF)
		set(DEFAULT_QT          OFF)
		set(DEFAULT_CEC         OFF)
	endif()
elseif ("${PLATFORM}" MATCHES "^x11")
	set(DEFAULT_X11        ON)
	set(DEFAULT_XCB        ON)
	if("${PLATFORM}" MATCHES "-dev$")
		set(DEFAULT_AMLOGIC    ON)
		set(DEFAULT_DEV_WS281XPWM  ON)
	endif()
elseif ("${PLATFORM}" STREQUAL "imx6")
	set(DEFAULT_FB         ON)
endif()

# enable tests for -dev builds
if("${PLATFORM}" MATCHES "-dev$")
	set(DEFAULT_TESTS      ON)
endif()

string(TOUPPER "-DPLATFORM_${PLATFORM}" PLATFORM_DEFINE)
string(REPLACE "-DEV" "" PLATFORM_DEFINE "${PLATFORM_DEFINE}")
ADD_DEFINITIONS(${PLATFORM_DEFINE})

#=============================================================================
# Set the build options
#=============================================================================

option(HYPERION_LIGHT "Build Hyperion with a reduced set of functionality" ${DEFAULT_HYPERION_LIGHT})
message(STATUS "HYPERION_LIGHT = ${HYPERION_LIGHT}")

if(HYPERION_LIGHT)
	message(STATUS "HYPERION_LIGHT: Hyperion is build with a reduced set of functionality.")
	# Disable Screen/Video Grabbers
	SET ( DEFAULT_AMLOGIC                     OFF )
	SET ( DEFAULT_DISPMANX                    OFF )
	SET ( DEFAULT_DX                          OFF )
	SET ( DEFAULT_DDA                         OFF )
	SET ( DEFAULT_FB                          OFF )
	SET ( DEFAULT_MF                          OFF )
	SET ( DEFAULT_OSX                         OFF )
	SET ( DEFAULT_QT                          OFF )
	SET ( DEFAULT_V4L2                        OFF )
	SET ( DEFAULT_X11                         OFF )
	SET ( DEFAULT_XCB                         OFF )

	# Disable Audio Grabbers
	SET ( DEFAULT_AUDIO                       OFF )

	# LED-Devices
	#SET ( DEFAULT_DEV_NETWORK                 OFF )
	#SET ( DEFAULT_DEV_FTDI                    OFF )
	#SET ( DEFAULT_DEV_SERIAL                  OFF )
	#SET ( DEFAULT_DEV_SPI                     OFF )
	#SET ( DEFAULT_DEV_TINKERFORGE             OFF )
	#SET ( DEFAULT_DEV_USB_HID                 OFF )
	#SET ( DEFAULT_DEV_WS281XPWM               OFF )

	# Disable Input Servers
	SET ( DEFAULT_BOBLIGHT_SERVER             OFF )
	SET ( DEFAULT_CEC                         OFF )
	SET ( DEFAULT_FLATBUF_SERVER              OFF )
	SET ( DEFAULT_PROTOBUF_SERVER             OFF )

	# Disable Output Connectors
	SET ( DEFAULT_FORWARDER                   OFF )
	SET ( DEFAULT_FLATBUF_CONNECT             OFF )

	# Disable Services
	SET ( DEFAULT_EFFECTENGINE                OFF )
	SET ( DEFAULT_EXPERIMENTAL                OFF )
	#SET ( DEFAULT_MDNS                       OFF )
	SET ( DEFAULT_REMOTE_CTL                  OFF )

	SET ( ENABLE_JSONCHECKS                    ON )
	SET ( ENABLE_DEPLOY_DEPENDENCIES           ON )
endif()
message(STATUS "Grabber options:")

addIndent(" - ")

option(ENABLE_AMLOGIC "Enable the AMLOGIC video grabber" ${DEFAULT_AMLOGIC})
message(STATUS "ENABLE_AMLOGIC = ${ENABLE_AMLOGIC}")

option(ENABLE_DISPMANX "Enable the RPi dispmanx grabber" ${DEFAULT_DISPMANX})
message(STATUS "ENABLE_DISPMANX = ${ENABLE_DISPMANX}")

option(ENABLE_DX "Enable the DirectX grabber" ${DEFAULT_DX})
message(STATUS "ENABLE_DX = ${ENABLE_DX}")

option(ENABLE_DDA "Enable the DXGI DDA grabber" ${DEFAULT_DDA})
message(STATUS "ENABLE_DDA = ${ENABLE_DDA}")

if(ENABLE_AMLOGIC)
	set(ENABLE_FB ON)
else()
	option(ENABLE_FB " Enable the framebuffer grabber" ${DEFAULT_FB})
endif()
message(STATUS "ENABLE_FB = ${ENABLE_FB}")

option(ENABLE_MF "Enable the Media Foundation grabber" ${DEFAULT_MF})
message(STATUS "ENABLE_MF = ${ENABLE_MF}")

option(ENABLE_OSX "Enable the OSX grabber" ${DEFAULT_OSX})
message(STATUS "ENABLE_OSX = ${ENABLE_OSX}")

option(ENABLE_QT "Enable the Qt grabber" ${DEFAULT_QT})
message(STATUS "ENABLE_QT = ${ENABLE_QT}")

option(ENABLE_V4L2 "Enable the V4L2 grabber" ${DEFAULT_V4L2})
message(STATUS "ENABLE_V4L2 = ${ENABLE_V4L2}")

option(ENABLE_X11 "Enable the X11 grabber" ${DEFAULT_X11})
message(STATUS "ENABLE_X11 = ${ENABLE_X11}")

option(ENABLE_XCB "Enable the XCB grabber" ${DEFAULT_XCB})
message(STATUS "ENABLE_XCB = ${ENABLE_XCB}")

option(ENABLE_AUDIO "Enable the AUDIO grabber" ${DEFAULT_AUDIO})
message(STATUS "ENABLE_AUDIO = ${ENABLE_AUDIO}")

removeIndent()

message(STATUS "Input options:")
addIndent(" - ")

option(ENABLE_BOBLIGHT_SERVER "Enable BOBLIGHT server" ${DEFAULT_BOBLIGHT_SERVER})
message(STATUS "ENABLE_BOBLIGHT_SERVER = ${ENABLE_BOBLIGHT_SERVER}")

option(ENABLE_CEC "Enable the libcec and CEC control" ${DEFAULT_CEC})
message(STATUS "ENABLE_CEC = ${ENABLE_CEC}")

option(ENABLE_FLATBUF_SERVER "Enable Flatbuffers server" ${DEFAULT_FLATBUF_SERVER})
message(STATUS "ENABLE_FLATBUF_SERVER = ${ENABLE_FLATBUF_SERVER}")

option(ENABLE_PROTOBUF_SERVER "Enable Protocol Buffers server" ${DEFAULT_PROTOBUF_SERVER})
message(STATUS "ENABLE_PROTOBUF_SERVER = ${ENABLE_PROTOBUF_SERVER}")

removeIndent()

message(STATUS "Output options:")
addIndent(" - ")

option(ENABLE_FORWARDER "Enable Hyperion forwarding" ${DEFAULT_FORWARDER})
message(STATUS "ENABLE_FORWARDER = ${ENABLE_FORWARDER}")

if(ENABLE_FORWARDER)
	set(ENABLE_FLATBUF_CONNECT ON)
else()
	option(ENABLE_FLATBUF_CONNECT "Enable Flatbuffers connecting remotely" ${DEFAULT_FLATBUF_CONNECT})
endif()
message(STATUS "ENABLE_FLATBUF_CONNECT = ${ENABLE_FLATBUF_CONNECT}")

removeIndent()

message(STATUS "LED-Device options:")
addIndent(" - ")

option(ENABLE_DEV_NETWORK "Enable the Network devices" ${DEFAULT_DEV_NETWORK})
message(STATUS "ENABLE_DEV_NETWORK = ${ENABLE_DEV_NETWORK}")

option(ENABLE_DEV_SERIAL "Enable the Serial devices" ${DEFAULT_DEV_SERIAL})
message(STATUS "ENABLE_DEV_SERIAL = ${ENABLE_DEV_SERIAL}")

option(ENABLE_DEV_SPI "Enable the SPI device" ${DEFAULT_DEV_SPI})
message(STATUS "ENABLE_DEV_SPI = ${ENABLE_DEV_SPI}")

option(ENABLE_DEV_TINKERFORGE "Enable the TINKERFORGE device" ${DEFAULT_DEV_TINKERFORGE})
message(STATUS "ENABLE_DEV_TINKERFORGE = ${ENABLE_DEV_TINKERFORGE}")

option(ENABLE_DEV_USB_HID "Enable the libusb and hid devices" ${DEFAULT_DEV_USB_HID})
message(STATUS "ENABLE_DEV_USB_HID = ${ENABLE_DEV_USB_HID}")

option(ENABLE_DEV_WS281XPWM "Enable the WS281x-PWM device" ${DEFAULT_DEV_WS281XPWM})
message(STATUS "ENABLE_DEV_WS281XPWM = ${ENABLE_DEV_WS281XPWM}")

option(ENABLE_DEV_FTDI "Enable the FTDI devices" ${DEFAULT_DEV_FTDI} )
message(STATUS "ENABLE_DEV_FTDI = ${ENABLE_DEV_FTDI}")

removeIndent()

message(STATUS "Services options:")
addIndent(" - ")

option(ENABLE_EFFECTENGINE "Enable Effect-Engine" ${DEFAULT_EFFECTENGINE})
message(STATUS "ENABLE_EFFECTENGINE = ${ENABLE_EFFECTENGINE}")

option(ENABLE_EXPERIMENTAL "Compile experimental features" ${DEFAULT_EXPERIMENTAL})
message(STATUS "ENABLE_EXPERIMENTAL = ${ENABLE_EXPERIMENTAL}")

option(ENABLE_MDNS "Enable mDNS (aka Zeroconf)" ${DEFAULT_MDNS})
message(STATUS "ENABLE_MDNS = ${ENABLE_MDNS}")

option(ENABLE_REMOTE_CTL "Enable Hyperion remote control" ${DEFAULT_REMOTE_CTL})
message(STATUS "ENABLE_REMOTE_CTL = ${ENABLE_REMOTE_CTL}")

removeIndent()

message(STATUS "Build options:")
addIndent(" - ")

option(ENABLE_JSONCHECKS "Validate json schema files" ${DEFAULT_JSONCHECKS})
message(STATUS "ENABLE_JSONCHECKS = ${ENABLE_JSONCHECKS}")

option(ENABLE_DEPLOY_DEPENDENCIES "Deploy with dependencies" ${DEFAULT_DEPLOY_DEPENDENCIES})
message(STATUS "ENABLE_DEPLOY_DEPENDENCIES = ${ENABLE_DEPLOY_DEPENDENCIES}")

if(ENABLE_FLATBUF_SERVER OR ENABLE_FLATBUF_CONNECT)
	message(STATUS "DEFAULT_USE_SYSTEM_FLATBUFFERS_LIBS = ${DEFAULT_USE_SYSTEM_FLATBUFFERS_LIBS}")
endif()

if(ENABLE_PROTOBUF_SERVER)
	message(STATUS "DEFAULT_USE_SYSTEM_PROTO_LIBS = ${DEFAULT_USE_SYSTEM_PROTO_LIBS}")
endif()

message(STATUS "DEFAULT_USE_SYSTEM_MBEDTLS_LIBS = ${DEFAULT_USE_SYSTEM_MBEDTLS_LIBS}")

if(ENABLE_MDNS)
	message(STATUS "DEFAULT_USE_SYSTEM_QMDNS_LIBS = ${DEFAULT_USE_SYSTEM_QMDNS_LIBS}")
endif()


option(ENABLE_PROFILER "enable profiler capabilities - not for release code" OFF)
message(STATUS "ENABLE_PROFILER = ${ENABLE_PROFILER}")

option(ENABLE_TESTS "Compile additional test applications" ${DEFAULT_TESTS})
message(STATUS "ENABLE_TESTS = ${ENABLE_TESTS}")

removeIndent()

#=============================================================================
# Configure the predefined header file to pass selected options to the code
#=============================================================================

configure_file("${PROJECT_SOURCE_DIR}/HyperionConfig.h.in" "${PROJECT_BINARY_DIR}/HyperionConfig.h")
include_directories(${PROJECT_BINARY_DIR})

# Setup header file cleanup job
if(NOT CMAKE_VERSION VERSION_LESS "3.15")
	set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_BINARY_DIR}/HyperionConfig.h)
else()
	set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_BINARY_DIR}/HyperionConfig.h)
endif()

#=============================================================================
# Check JSON files/schemas
#=============================================================================

if(ENABLE_JSONCHECKS OR ENABLE_EFFECTENGINE)
	if(CMAKE_VERSION VERSION_LESS "3.12")
		set(Python_ADDITIONAL_VERSIONS 3.5)
		find_package(PythonInterp 3.5 REQUIRED)
	else()
		find_package(Python3 3.5 REQUIRED)
		set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
	endif()
endif()

if(ENABLE_JSONCHECKS)
	# check all json files
	file (GLOB_RECURSE HYPERION_SCHEMAS RELATIVE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/libsrc/*schema*.json)
	set(JSON_FILES ${CMAKE_BINARY_DIR}/settings/hyperion.settings.json.default	${HYPERION_SCHEMAS})

	execute_process (
		COMMAND ${PYTHON_EXECUTABLE} test/jsonchecks/checkjson.py ${JSON_FILES}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		RESULT_VARIABLE CHECK_JSON_FAILED
	)

	if(${CHECK_JSON_FAILED})
		message (FATAL_ERROR "check of json files failed")
	endif()

	if(ENABLE_EFFECTENGINE)
		execute_process (
			COMMAND ${PYTHON_EXECUTABLE} test/jsonchecks/checkeffects.py effects effects/schema
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			RESULT_VARIABLE CHECK_EFFECTS_FAILED
		)
		if(${CHECK_EFFECTS_FAILED})
			message (FATAL_ERROR "check of json effect files failed")
		endif()
	endif()

	execute_process (
		COMMAND ${PYTHON_EXECUTABLE} test/jsonchecks/checkschema.py ${CMAKE_BINARY_DIR}/settings/hyperion.settings.json.default libsrc/hyperion/schema/schema-settings-default.json
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		RESULT_VARIABLE CHECK_CONFIG_FAILED
	)

	if(${CHECK_CONFIG_FAILED})
		message (FATAL_ERROR "check of json default config failed")
	endif()
endif()

#=============================================================================
# Platform-specific tasks
#=============================================================================

# Search for Windows SDK
if(MSVC)
	find_package(WindowsSDK REQUIRED)
	message(STATUS "WINDOWS SDK: ${WINDOWSSDK_LATEST_DIR} ${WINDOWSSDK_LATEST_NAME}")
	message(STATUS "MSVC VERSION: ${MSVC_VERSION}")
endif()

# If Hyperion is deployed with all dependencies under Linux
# it is necessary to deactivate the linker option dtag
if(ENABLE_DEPLOY_DEPENDENCIES AND SYSTEM_MATCHES_LINUX)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--disable-new-dtags")
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--disable-new-dtags")
	set(CMAKE_SKIP_BUILD_RPATH OFF)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
	set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
endif()

if(APPLE)
	set(CMAKE_EXE_LINKER_FLAGS "-framework CoreGraphics")
endif()

find_package(Threads REQUIRED)
find_package(libusb-1.0 REQUIRED)

#=============================================================================
# Allow to overwrite QT base directory
# Either supply QTDIR as -DQTDIR=<path> to cmake or set and environment variable QTDIR pointing to the Qt installation
# For Windows and macOS, the default Qt installation path are tried to resolved automatically
#=============================================================================

if(NOT DEFINED QTDIR)
	if(DEFINED ENV{QTDIR})
		set(QTDIR $ENV{QTDIR})
	else()
		if(MSVC)
			FIRSTSUBDIR(SUBDIRQT "C:/Qt")
			if(NOT ${SUBDIRQT} STREQUAL "")
				set(QTDIR "${SUBDIRQT}/msvc2019_64")
			endif()
		elseif ("${PLATFORM}" MATCHES "osx")
			foreach(QT_VERSION 6 5)
				execute_process(
					COMMAND brew --prefix qt@${QT_VERSION}
					RESULT_VARIABLE DETECT_QT
					OUTPUT_VARIABLE QT_LOCATION
					ERROR_QUIET
					OUTPUT_STRIP_TRAILING_WHITESPACE
				)
				if(${DETECT_QT} EQUAL 0 AND EXISTS ${QT_LOCATION})
					set(QTDIR ${QT_LOCATION})
					break()
				endif()
			endforeach()
		endif()
	endif()
endif()

if(DEFINED QTDIR)
	message(STATUS "Add QTDIR: ${QTDIR} to CMAKE_PREFIX_PATH")
	list(PREPEND CMAKE_PREFIX_PATH
		${QTDIR}
		${QTDIR}/lib
	)
endif()

if(CMAKE_PREFIX_PATH)
	message(STATUS "CMAKE_PREFIX_PATH used: ${CMAKE_PREFIX_PATH}")
endif()

# find QT libs
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Network Sql Widgets REQUIRED)
message(STATUS "Found Qt Version: ${QT_VERSION}")

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
	set(QT_MIN_VERSION "6.2.2")
else()
	set(QT_MIN_VERSION "5.9.0")
endif()

if("${QT_VERSION}" VERSION_LESS "${QT_MIN_VERSION}")
	message(FATAL_ERROR "Your Qt version is to old! Minimum required ${QT_MIN_VERSION}")
endif()

find_package(Qt${QT_VERSION_MAJOR} ${QT_VERSION} COMPONENTS Core Gui Network Sql Widgets REQUIRED)

message(STATUS "Qt version used: ${QT_VERSION}")

if(APPLE AND (${QT_VERSION_MAJOR} GREATER_EQUAL 6))
	set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
endif()

#=============================================================================
# Add our Code
#=============================================================================

# Add the source/lib directories
add_subdirectory(dependencies)
add_subdirectory(resources)
add_subdirectory(libsrc)
add_subdirectory(src)
if(ENABLE_TESTS)
	add_subdirectory(test)
endif()

#=============================================================================
# Create uninstall target
#=============================================================================

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

#=============================================================================
# Add Packaging
#=============================================================================

include(Packaging)
