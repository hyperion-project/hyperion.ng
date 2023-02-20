cmake_minimum_required(VERSION 3.1.0)

message( STATUS "CMake Version: ${CMAKE_VERSION}" )

macro(addIndent text)
if(${CMAKE_VERSION} VERSION_GREATER "3.16.0")
	list(APPEND CMAKE_MESSAGE_INDENT ${text})
endif()
endmacro()

macro(removeIndent)
if(${CMAKE_VERSION} VERSION_GREATER "3.16.0")
	list(POP_BACK CMAKE_MESSAGE_INDENT)
endif()
endmacro()

PROJECT(hyperion)

# Parse semantic version of version file and write version to config
include (${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake)
file (STRINGS ".version" HYPERION_VERSION)
SetVersionNumber(HYPERION ${HYPERION_VERSION})
set(DEFAULT_JSON_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/config/hyperion.config.json.default)
file(READ ${DEFAULT_JSON_CONFIG_FILE} DEFAULT_JSON_CONFIG_VAR)
string(REPLACE "configVersionValue" ${HYPERION_VERSION} DEFAULT_JSON_CONFIG_VAR "${DEFAULT_JSON_CONFIG_VAR}")
string(REPLACE "previousVersionValue" ${HYPERION_VERSION} DEFAULT_JSON_CONFIG_VAR "${DEFAULT_JSON_CONFIG_VAR}")
file(WRITE ${CMAKE_BINARY_DIR}/config/hyperion.config.json.default "${DEFAULT_JSON_CONFIG_VAR}")

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# auto prepare .qrc files
set(CMAKE_AUTORCC ON)

# Configure CCache if available
find_program(CCACHE_FOUND ccache)
if ( CCACHE_FOUND )
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

# enable C++14; MSVC doesn't have c++14 feature switch
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	if(APPLE)
		include(CheckCXXCompilerFlag)
		CHECK_CXX_COMPILER_FLAG("Werror=unguarded-availability" REQUIRED_UNGUARDED_AVAILABILITY)
		if(REQUIRED_UNGUARDED_AVAILABILITY)
			list(APPEND CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS} "Werror=unguarded-availability")
		endif()
	endif()

	if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-psabi")
	endif()

	set(CMAKE_CXX_STANDARD 14)
	set(CXX_STANDARD_REQUIRED ON)
	set(CXX_EXTENSIONS OFF)
endif()

# Set build variables
# Grabber
SET ( DEFAULT_AMLOGIC                     OFF )
SET ( DEFAULT_DISPMANX                    OFF )
SET ( DEFAULT_DX                          OFF )
SET ( DEFAULT_MF                          OFF )
SET ( DEFAULT_OSX                         OFF )
SET ( DEFAULT_QT                          ON  )
SET ( DEFAULT_V4L2                        OFF )
SET ( DEFAULT_AUDIO                       ON  )
SET ( DEFAULT_X11                         OFF )
SET ( DEFAULT_XCB                         OFF )

# Input
SET ( DEFAULT_BOBLIGHT_SERVER             ON  )
SET ( DEFAULT_CEC                         OFF )
SET ( DEFAULT_FLATBUF_SERVER              ON  )
SET ( DEFAULT_PROTOBUF_SERVER             ON  )

# Output
SET ( DEFAULT_FORWARDER                   ON  )
SET ( DEFAULT_FLATBUF_CONNECT             ON  )

# LED-Devices
SET ( DEFAULT_DEV_NETWORK                 ON  )
SET ( DEFAULT_DEV_SERIAL                  ON  )
SET ( DEFAULT_DEV_SPI                     OFF )
SET ( DEFAULT_DEV_TINKERFORGE             OFF )
SET ( DEFAULT_DEV_USB_HID                 OFF )
SET ( DEFAULT_DEV_WS281XPWM               OFF )

# Services
SET ( DEFAULT_EFFECTENGINE                ON  )
SET ( DEFAULT_EXPERIMENTAL                OFF )
SET ( DEFAULT_MDNS                        ON  )
SET ( DEFAULT_REMOTE_CTL                  ON  )

# Build
SET ( DEFAULT_JSONCHECKS                  ON  )
SET ( DEFAULT_DEPLOY_DEPENDENCIES         ON  )
SET ( DEFAULT_USE_SYSTEM_FLATBUFFERS_LIBS OFF )
SET ( DEFAULT_USE_SYSTEM_PROTO_LIBS       OFF )
SET ( DEFAULT_USE_SYSTEM_MBEDTLS_LIBS     OFF )
SET ( DEFAULT_USE_SYSTEM_QMDNS_LIBS       OFF )
SET ( DEFAULT_TESTS                       OFF )

# Build Hyperion with a reduced set of functionality, overwrites other default values
SET ( DEFAULT_HYPERION_LIGHT              OFF )

IF ( ${CMAKE_SYSTEM} MATCHES "Linux" )
	SET ( DEFAULT_FB              ON )
	SET ( DEFAULT_V4L2            ON )
	SET ( DEFAULT_DEV_SPI         ON )
	SET ( DEFAULT_DEV_TINKERFORGE ON )
	SET ( DEFAULT_DEV_USB_HID     ON )
	SET ( DEFAULT_CEC             ON )
ELSEIF ( WIN32 )
	SET ( DEFAULT_DX ON )
	SET ( DEFAULT_MF ON )
ELSE()
	SET ( DEFAULT_FB              OFF )
	SET ( DEFAULT_V4L2            OFF )
	SET ( DEFAULT_DEV_SPI         OFF )
	SET ( DEFAULT_DEV_TINKERFORGE OFF )
	SET ( DEFAULT_DEV_USB_HID     OFF )
	SET ( DEFAULT_CEC             OFF )
ENDIF()

if ( NOT DEFINED PLATFORM )
	if ( APPLE )
		SET( PLATFORM "osx")
	elseif ( WIN32 )
		SET( PLATFORM "windows")
	elseif ( "${CMAKE_SYSTEM_PROCESSOR}" MATCHES "x86" )
		SET( PLATFORM "x11")
	elseif ( "${CMAKE_SYSTEM_PROCESSOR}" MATCHES "arm" OR "${CMAKE_SYSTEM_PROCESSOR}" MATCHES "aarch64")
		SET( PLATFORM "rpi")
		FILE( READ /proc/cpuinfo SYSTEM_CPUINFO )
		STRING ( TOLOWER "${SYSTEM_CPUINFO}" SYSTEM_CPUINFO )
		if ( "${SYSTEM_CPUINFO}" MATCHES "amlogic" AND ${CMAKE_SIZEOF_VOID_P} EQUAL 4 )
			SET( PLATFORM "amlogic" )
		elseif ( ("${SYSTEM_CPUINFO}" MATCHES "amlogic" OR "${SYSTEM_CPUINFO}" MATCHES "odroid-c2" OR "${SYSTEM_CPUINFO}" MATCHES "vero4k") AND ${CMAKE_SIZEOF_VOID_P} EQUAL 8 )
			SET( PLATFORM "amlogic64" )
		endif()
	endif()
	if ( PLATFORM )
		message( STATUS "PLATFORM is not defined, evaluated platform: ${PLATFORM}")
	else()
		message( FATAL_ERROR "PLATFORM is not defined and could not be evaluated. Set -DPLATFORM=<rpi|amlogic|amlogic64|x11|x11-dev|osx|osx-dev>")
	endif()
endif()

message( STATUS "PLATFORM: ${PLATFORM}")

# Macro to get path of first sub dir of a dir, used for MAC OSX lib/header searching
MACRO(FIRSTSUBDIR result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
	IF(IS_DIRECTORY ${curdir}/${child})
	  LIST(APPEND dirlist "${curdir}/${child}")
		BREAK()
	ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

if ( "${PLATFORM}" MATCHES "osx" )
	# specify the min version of the target platform
	SET ( CMAKE_OSX_DEPLOYMENT_TARGET "10.15" )
	# add specific prefix paths
	FIRSTSUBDIR(SUBDIRPY "/usr/local/opt/python3/Frameworks/Python.framework/Versions")
	set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${SUBDIRPY})

	include_directories("/opt/X11/include/")
	SET ( DEFAULT_OSX         ON  )
	SET ( DEFAULT_AUDIO       OFF )
	SET ( DEFAULT_DEV_USB_HID ON  )

elseif ( "${PLATFORM}" MATCHES "rpi" )
	SET ( DEFAULT_DISPMANX   ON )
	SET ( DEFAULT_DEV_WS281XPWM  ON )
elseif ( "${PLATFORM}" STREQUAL "amlogic" )
	SET ( DEFAULT_AMLOGIC     ON )
elseif ( "${PLATFORM}" STREQUAL "amlogic-dev" )
	SET ( DEFAULT_AMLOGIC     ON )
	SET ( DEFAULT_DISPMANX    OFF )
	SET ( DEFAULT_QT          OFF )
	SET ( DEFAULT_CEC         OFF )
elseif ( "${PLATFORM}" STREQUAL "amlogic64" )
	SET ( DEFAULT_AMLOGIC     ON )
elseif ( "${PLATFORM}" MATCHES "x11" )
	SET ( DEFAULT_X11        ON )
	SET ( DEFAULT_XCB        ON )
	if ( "${PLATFORM}" STREQUAL "x11-dev" )
		SET ( DEFAULT_AMLOGIC    ON)
		SET ( DEFAULT_DEV_WS281XPWM  ON )
	endif()
elseif ( "${PLATFORM}" STREQUAL "imx6" )
	SET ( DEFAULT_FB         ON )
endif()

# enable tests for -dev builds
if ( "${PLATFORM}" MATCHES "-dev" )
	SET ( DEFAULT_TESTS      ON )
endif()

STRING( TOUPPER "-DPLATFORM_${PLATFORM}" PLATFORM_DEFINE)
STRING( REPLACE "-DEV" "" PLATFORM_DEFINE "${PLATFORM_DEFINE}" )
ADD_DEFINITIONS( ${PLATFORM_DEFINE} )

# set the build options

option(HYPERION_LIGHT "Build Hyperion with a reduced set of functionality" ${DEFAULT_HYPERION_LIGHT} )
message(STATUS "HYPERION_LIGHT = ${HYPERION_LIGHT}")

if (HYPERION_LIGHT)
	message(STATUS "HYPERION_LIGHT: Hyperion is build with a reduced set of functionality.")
	# Disable Grabbers
	SET ( DEFAULT_AMLOGIC                     OFF )
	SET ( DEFAULT_DISPMANX                    OFF )
	SET ( DEFAULT_DX                          OFF )
	SET ( DEFAULT_FB                          OFF )
	SET ( DEFAULT_MF                          OFF )
	SET ( DEFAULT_OSX                         OFF )
	SET ( DEFAULT_QT                          OFF )
	SET ( DEFAULT_V4L2                        OFF )
	SET ( DEFAULT_AUDIO                       OFF )
	SET ( DEFAULT_X11                         OFF )
	SET ( DEFAULT_XCB                         OFF )

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
endif()

message(STATUS "Grabber options:")

addIndent(" - ")

option(ENABLE_AMLOGIC "Enable the AMLOGIC video grabber" ${DEFAULT_AMLOGIC} )
message(STATUS "ENABLE_AMLOGIC = ${ENABLE_AMLOGIC}")

option(ENABLE_DISPMANX "Enable the RPi dispmanx grabber" ${DEFAULT_DISPMANX} )
message(STATUS "ENABLE_DISPMANX = ${ENABLE_DISPMANX}")

option(ENABLE_DX "Enable the DirectX grabber" ${DEFAULT_DX})
message(STATUS "ENABLE_DX = ${ENABLE_DX}")

if (ENABLE_AMLOGIC)
	SET(ENABLE_FB ON)
else()
	option(ENABLE_FB " Enable the framebuffer grabber" ${DEFAULT_FB} )
endif()
message(STATUS "ENABLE_FB = ${ENABLE_FB}")

option(ENABLE_MF "Enable the Media Foundation grabber" ${DEFAULT_MF})
message(STATUS "ENABLE_MF = ${ENABLE_MF}")

option(ENABLE_OSX "Enable the OSX grabber" ${DEFAULT_OSX} )
message(STATUS "ENABLE_OSX = ${ENABLE_OSX}")

option(ENABLE_QT "Enable the Qt grabber" ${DEFAULT_QT})
message(STATUS "ENABLE_QT = ${ENABLE_QT}")

option(ENABLE_V4L2 "Enable the V4L2 grabber" ${DEFAULT_V4L2})
message(STATUS "ENABLE_V4L2 = ${ENABLE_V4L2}")

option(ENABLE_X11 "Enable the X11 grabber" ${DEFAULT_X11})
message(STATUS "ENABLE_X11 = ${ENABLE_X11}")
option(ENABLE_AUDIO "Enable the AUDIO grabber" ${DEFAULT_AUDIO})
message(STATUS "ENABLE_AUDIO = ${ENABLE_AUDIO}")

option(ENABLE_WS281XPWM "Enable the WS281x-PWM device" ${DEFAULT_WS281XPWM} )
message(STATUS "ENABLE_WS281XPWM = ${ENABLE_WS281XPWM}")

option(ENABLE_XCB "Enable the XCB grabber" ${DEFAULT_XCB})
message(STATUS "ENABLE_XCB = ${ENABLE_XCB}")

removeIndent()

message(STATUS "Input options:")
addIndent(" - ")

option(ENABLE_BOBLIGHT_SERVER "Enable BOBLIGHT server" ${DEFAULT_BOBLIGHT_SERVER} )
message(STATUS "ENABLE_BOBLIGHT_SERVER = ${ENABLE_BOBLIGHT_SERVER}")

option(ENABLE_CEC "Enable the libcec and CEC control" ${DEFAULT_CEC} )
message(STATUS "ENABLE_CEC = ${ENABLE_CEC}")

option(ENABLE_FLATBUF_SERVER "Enable Flatbuffers server" ${DEFAULT_FLATBUF_SERVER} )
message(STATUS "ENABLE_FLATBUF_SERVER = ${ENABLE_FLATBUF_SERVER}")

option(ENABLE_PROTOBUF_SERVER "Enable Protocol Buffers server" ${DEFAULT_PROTOBUF_SERVER} )
message(STATUS "ENABLE_PROTOBUF_SERVER = ${ENABLE_PROTOBUF_SERVER}")

removeIndent()

message(STATUS "Output options:")
addIndent(" - ")

option(ENABLE_FORWARDER "Enable Hyperion forwarding" ${DEFAULT_FORWARDER} )
message(STATUS "ENABLE_FORWARDER = ${ENABLE_FORWARDER}")

if (ENABLE_FORWARDER)
	SET(ENABLE_FLATBUF_CONNECT ON)
else()
	option(ENABLE_FLATBUF_CONNECT "Enable Flatbuffers connecting remotely" ${DEFAULT_FLATBUF_CONNECT} )
endif()
message(STATUS "ENABLE_FLATBUF_CONNECT = ${ENABLE_FLATBUF_CONNECT}")

removeIndent()

message(STATUS "LED-Device options:")
addIndent(" - ")

option(ENABLE_DEV_NETWORK "Enable the Network devices" ${DEFAULT_DEV_NETWORK} )
message(STATUS "ENABLE_DEV_NETWORK = ${ENABLE_DEV_NETWORK}")

option(ENABLE_DEV_SERIAL "Enable the Serial devices" ${DEFAULT_DEV_SERIAL} )
message(STATUS "ENABLE_DEV_SERIAL = ${ENABLE_DEV_SERIAL}")

option(ENABLE_DEV_SPI "Enable the SPI device" ${DEFAULT_DEV_SPI} )
message(STATUS "ENABLE_DEV_SPI = ${ENABLE_DEV_SPI}")

option(ENABLE_DEV_TINKERFORGE "Enable the TINKERFORGE device" ${DEFAULT_DEV_TINKERFORGE})
message(STATUS "ENABLE_DEV_TINKERFORGE = ${ENABLE_DEV_TINKERFORGE}")

option(ENABLE_DEV_USB_HID "Enable the libusb and hid devices" ${DEFAULT_DEV_USB_HID} )
message(STATUS "ENABLE_DEV_USB_HID = ${ENABLE_DEV_USB_HID}")

option(ENABLE_DEV_WS281XPWM "Enable the WS281x-PWM device" ${DEFAULT_DEV_WS281XPWM} )
message(STATUS "ENABLE_DEV_WS281XPWM = ${ENABLE_DEV_WS281XPWM}")

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

if (ENABLE_MDNS)
	message(STATUS "DEFAULT_USE_SYSTEM_QMDNS_LIBS = ${DEFAULT_USE_SYSTEM_QMDNS_LIBS}")
endif()


option(ENABLE_PROFILER "enable profiler capabilities - not for release code" OFF)
message(STATUS "ENABLE_PROFILER = ${ENABLE_PROFILER}")

option(ENABLE_TESTS "Compile additional test applications" ${DEFAULT_TESTS})
message(STATUS "ENABLE_TESTS = ${ENABLE_TESTS}")

removeIndent()

SET ( FLATBUFFERS_INSTALL_BIN_DIR ${CMAKE_BINARY_DIR}/flatbuf )
SET ( FLATBUFFERS_INSTALL_LIB_DIR ${CMAKE_BINARY_DIR}/flatbuf )

SET ( PROTOBUF_INSTALL_BIN_DIR ${CMAKE_BINARY_DIR}/proto )
SET ( PROTOBUF_INSTALL_LIB_DIR ${CMAKE_BINARY_DIR}/proto )

if(ENABLE_JSONCHECKS OR ENABLE_EFFECTENGINE)
	if ("${CMAKE_VERSION}" VERSION_LESS "3.12.0")
			set(Python_ADDITIONAL_VERSIONS 3.5)
			find_package(PythonInterp 3.5 REQUIRED)
	else()
		find_package(Python3 3.5 COMPONENTS Interpreter Development REQUIRED)
		if(Python3_FOUND)
			set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
		endif()
	endif()
endif()

if(ENABLE_JSONCHECKS)
	# check all json files
	FILE ( GLOB_RECURSE HYPERION_SCHEMAS RELATIVE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/libsrc/*schema*.json )
	SET( JSON_FILES ${CMAKE_BINARY_DIR}/config/hyperion.config.json.default	${HYPERION_SCHEMAS})

	EXECUTE_PROCESS (
		COMMAND ${PYTHON_EXECUTABLE} test/jsonchecks/checkjson.py ${JSON_FILES}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		RESULT_VARIABLE CHECK_JSON_FAILED
	)
	IF ( ${CHECK_JSON_FAILED} )
		MESSAGE (FATAL_ERROR "check of json files failed" )
	ENDIF ()

	if(ENABLE_EFFECTENGINE)
		EXECUTE_PROCESS (
			COMMAND ${PYTHON_EXECUTABLE} test/jsonchecks/checkeffects.py effects effects/schema
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			RESULT_VARIABLE CHECK_EFFECTS_FAILED
		)
		IF ( ${CHECK_EFFECTS_FAILED} )
			MESSAGE (FATAL_ERROR "check of json effect files failed" )
		ENDIF ()
	endif()

	EXECUTE_PROCESS (
		COMMAND ${PYTHON_EXECUTABLE} test/jsonchecks/checkschema.py ${CMAKE_BINARY_DIR}/config/hyperion.config.json.default libsrc/hyperion/hyperion.schema.json
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		RESULT_VARIABLE CHECK_CONFIG_FAILED
	)

	IF ( ${CHECK_CONFIG_FAILED} )
		MESSAGE (FATAL_ERROR "check of json default config failed" )
	ENDIF ()
endif(ENABLE_JSONCHECKS)

# Add project specific cmake modules (find, etc)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(GitVersion)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file("${PROJECT_SOURCE_DIR}/HyperionConfig.h.in" "${PROJECT_BINARY_DIR}/HyperionConfig.h")
include_directories("${PROJECT_BINARY_DIR}")

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

# MSVC options
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	# Search for Windows SDK
	find_package(WindowsSDK REQUIRED)
	message(STATUS "WINDOWS SDK: ${WINDOWSSDK_LATEST_DIR} ${WINDOWSSDK_LATEST_NAME}")
	message(STATUS "MSVC VERSION: ${MSVC_VERSION}")

	# Search for DirectX9
	if (ENABLE_DX)
		find_package(DirectX9 REQUIRED)
	endif(ENABLE_DX)

endif()

# Use GNU gold linker if available
if (NOT WIN32 AND NOT APPLE)
	include (${CMAKE_CURRENT_SOURCE_DIR}/cmake/LDGold.cmake)
endif()

# Don't create new dynamic tags (RUNPATH) and setup -rpath to search for shared libs in BINARY/../lib folder (only for Unix)
if (ENABLE_DEPLOY_DEPENDENCIES AND UNIX AND NOT APPLE)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")
	SET(CMAKE_SKIP_BUILD_RPATH FALSE)
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
	SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:$ORIGIN/../lib")
	SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif ()

if(APPLE)
	set(CMAKE_EXE_LINKER_FLAGS "-framework CoreGraphics")
endif()

find_package(Threads REQUIRED)

# Allow to overwrite QT base directory
# Either supply QTDIR as -DQTDIR=<path> to cmake or set and environment variable QTDIR pointing to the Qt installation
# For Windows and OSX, the default Qt installation path are tried to resolved automatically
if (NOT DEFINED QTDIR)
	if (DEFINED ENV{QTDIR})
		set(QTDIR $ENV{QTDIR})
	else()
		if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
			FIRSTSUBDIR(SUBDIRQT "C:/Qt")
			if (NOT ${SUBDIRQT} STREQUAL "")
				set(QTDIR "${SUBDIRQT}/msvc2019_64")
			endif()
		elseif ( "${PLATFORM}" MATCHES "osx" )
			# QT6 x86_64 location
			if (EXISTS /usr/local/opt/qt6)
				set(QTDIR "/usr/local/opt/qt6")
			# QT6 arm64 location
			elseif (EXISTS /opt/homebrew/opt/qt@6)
				set(QTDIR "/opt/homebrew/opt/qt@6")
			# QT5 x86_64 location
			elseif (EXISTS /usr/local/opt/qt5)
				set(QTDIR "/usr/local/opt/qt5")
			# QT5 arm64 location
			elseif (EXISTS /opt/homebrew/opt/qt@5)
				set(QTDIR "/opt/homebrew/opt/qt@5")
			endif()
		endif()
	endif()
endif()

if (DEFINED QTDIR)
	message(STATUS "Add QTDIR: ${QTDIR} to CMAKE_PREFIX_PATH")
	list(PREPEND CMAKE_PREFIX_PATH ${QTDIR} "${QTDIR}/lib")
endif()

if (CMAKE_PREFIX_PATH)
	message( STATUS "CMAKE_PREFIX_PATH used: ${CMAKE_PREFIX_PATH}" )
endif()

# find QT libs
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Network Sql Widgets REQUIRED)
message( STATUS "Found Qt Version: ${QT_VERSION}" )

if (${QT_VERSION_MAJOR} GREATER_EQUAL 6 )
	SET(QT_MIN_VERSION "6.2.2")
else()
	SET(QT_MIN_VERSION "5.5.0")
endif()

if ( "${QT_VERSION}" VERSION_LESS "${QT_MIN_VERSION}" )
	message( FATAL_ERROR "Your Qt version is to old! Minimum required ${QT_MIN_VERSION}" )
endif()

find_package(Qt${QT_VERSION_MAJOR} ${QT_VERSION} COMPONENTS Core Gui Network Sql Widgets REQUIRED)

message( STATUS "Qt version used: ${QT_VERSION}" )

if (APPLE AND (${QT_VERSION_MAJOR} GREATER_EQUAL 6) )
	set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
endif()

# Add libusb and pthreads
find_package(libusb-1.0 REQUIRED)
add_definitions(${QT_DEFINITIONS})

# Add the source/lib directories
add_subdirectory(dependencies)
add_subdirectory(libsrc)
add_subdirectory(src)
if (ENABLE_TESTS)
	add_subdirectory(test)
endif ()

# Add resources directory
add_subdirectory(resources)

# remove generated files on make cleaan too
LIST( APPEND GENERATED_QRC
	${CMAKE_BINARY_DIR}/WebConfig.qrc
	${CMAKE_BINARY_DIR}/HyperionConfig.h
)

if(ENABLE_EFFECTENGINE)
LIST( APPEND GENERATED_QRC
	${CMAKE_BINARY_DIR}/EffectEngine.qrc
)
endif ()

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${GENERATED_QRC}" )

# uninstall target
configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

# enable make package - no code after this line !
include (${CMAKE_CURRENT_SOURCE_DIR}/cmake/packages.cmake)
