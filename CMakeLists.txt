cmake_minimum_required(VERSION 3.0.0)

message( STATUS "CMake Version: ${CMAKE_VERSION}" )

PROJECT(hyperion)

# Parse semantic version of version file
include (${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake)
file (STRINGS "version" HYPERION_VERSION)
SetVersionNumber(HYPERION ${HYPERION_VERSION})

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# auto prepare .qrc files
set(CMAKE_AUTORCC ON)

if ( POLICY CMP0026 )
	CMAKE_POLICY( SET CMP0026 OLD )
endif()

# Configure CCache if available
find_program(CCACHE_FOUND ccache)
if ( CCACHE_FOUND )
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

if (NOT CMAKE_VERSION VERSION_LESS "3.12")
	find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
	if(Python_FOUND)
		set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
	endif()
else()
	set(Python_ADDITIONAL_VERSIONS 3.5)
	find_package( PythonInterp 3.5 REQUIRED )
endif()

# Set build variables
SET ( DEFAULT_AMLOGIC                     OFF )
SET ( DEFAULT_DISPMANX                    OFF )
SET ( DEFAULT_OSX                         OFF )
SET ( DEFAULT_X11                         OFF )
SET ( DEFAULT_XCB                         OFF )
SET ( DEFAULT_QT                          ON  )
SET ( DEFAULT_WS281XPWM                   OFF )
SET ( DEFAULT_AVAHI                       ON  )
SET ( DEFAULT_USE_SHARED_AVAHI_LIBS       ON  )
SET ( DEFAULT_USE_SYSTEM_FLATBUFFERS_LIBS OFF )
SET ( DEFAULT_USE_SYSTEM_PROTO_LIBS       OFF )
SET ( DEFAULT_USE_SYSTEM_MBEDTLS_LIBS     OFF )
SET ( DEFAULT_TESTS                       OFF )
SET ( DEFAULT_EXPERIMENTAL                OFF )

IF ( ${CMAKE_SYSTEM} MATCHES "Linux" )
	SET ( DEFAULT_V4L2        ON )
	SET ( DEFAULT_SPIDEV      ON )
	SET ( DEFAULT_TINKERFORGE ON )
	SET ( DEFAULT_FB          ON )
	SET ( DEFAULT_USB_HID     ON )
	SET ( DEFAULT_CEC         ON )
ELSE()
	SET ( DEFAULT_V4L2        OFF )
	SET ( DEFAULT_FB          OFF )
	SET ( DEFAULT_SPIDEV      OFF )
	SET ( DEFAULT_TINKERFORGE OFF )
	SET ( DEFAULT_USB_HID     OFF )
	SET ( DEFAULT_CEC         OFF )
ENDIF()

if ( NOT DEFINED PLATFORM )
	if ( "${CMAKE_SYSTEM_PROCESSOR}" MATCHES "x86" )
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
	elseif ( APPLE )
		SET( PLATFORM "osx")
	elseif ( WIN32 )
		SET( PLATFORM "windows")
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
	# add specific prefix paths
	FIRSTSUBDIR(SUBDIRQT "/usr/local/Cellar/qt5")
	FIRSTSUBDIR(SUBDIRPY "/usr/local/opt/python3/Frameworks/Python.framework/Versions")
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${SUBDIRQT} ${SUBDIRPY} "/usr/local/opt/qt5" )
	include_directories("/opt/X11/include/")
	SET ( DEFAULT_OSX        ON )
	SET ( DEFAULT_USB_HID    ON )
elseif ( "${PLATFORM}" MATCHES "rpi" )
	SET ( DEFAULT_DISPMANX   ON )
	SET ( DEFAULT_WS281XPWM  ON )
elseif ( "${PLATFORM}" STREQUAL "amlogic" )
	SET ( DEFAULT_AMLOGIC    ON )
elseif ( "${PLATFORM}" STREQUAL "amlogic64" )
	SET ( DEFAULT_AMLOGIC    ON )
elseif ( "${PLATFORM}" MATCHES "x11" )
	SET ( DEFAULT_X11        ON )
	SET ( DEFAULT_XCB        ON )
	if ( "${PLATFORM}" STREQUAL "x11-dev" )
		SET ( DEFAULT_AMLOGIC    ON)
		SET ( DEFAULT_WS281XPWM  ON )
	endif()
elseif ( "${PLATFORM}" STREQUAL "imx6" )
	SET ( DEFAULT_FB         ON )
elseif (WIN32)
	SET ( DEFAULT_AVAHI    OFF)
endif()

# enable tests for -dev builds
if ( "${PLATFORM}" MATCHES "-dev" )
	SET ( DEFAULT_TESTS      ON )
endif()

STRING( TOUPPER "-DPLATFORM_${PLATFORM}" PLATFORM_DEFINE)
STRING( REPLACE "-DEV" "" PLATFORM_DEFINE "${PLATFORM_DEFINE}" )
ADD_DEFINITIONS( ${PLATFORM_DEFINE} )

# set the build options
option(ENABLE_AMLOGIC "Enable the AMLOGIC video grabber" ${DEFAULT_AMLOGIC} )
message(STATUS "ENABLE_AMLOGIC = ${ENABLE_AMLOGIC}")

option(ENABLE_DISPMANX "Enable the RPi dispmanx grabber" ${DEFAULT_DISPMANX} )
message(STATUS "ENABLE_DISPMANX = ${ENABLE_DISPMANX}")

if (ENABLE_AMLOGIC)
	SET(ENABLE_FB ON)
else()
	option(ENABLE_FB "Enable the framebuffer grabber" ${DEFAULT_FB} )
endif()
message(STATUS "ENABLE_FB = ${ENABLE_FB}")

option(ENABLE_OSX "Enable the osx grabber" ${DEFAULT_OSX} )
message(STATUS "ENABLE_OSX = ${ENABLE_OSX}")

option(ENABLE_SPIDEV   "Enable the SPIDEV device" ${DEFAULT_SPIDEV} )
message(STATUS "ENABLE_SPIDEV   = ${ENABLE_SPIDEV}")

option(ENABLE_TINKERFORGE "Enable the TINKERFORGE device" ${DEFAULT_TINKERFORGE})
message(STATUS "ENABLE_TINKERFORGE = ${ENABLE_TINKERFORGE}")

option(ENABLE_V4L2 "Enable the V4L2 grabber" ${DEFAULT_V4L2})
message(STATUS "ENABLE_V4L2 = ${ENABLE_V4L2}")

option(ENABLE_WS281XPWM "Enable the WS281x-PWM device" ${DEFAULT_WS281XPWM} )
message(STATUS "ENABLE_WS281XPWM = ${ENABLE_WS281XPWM}")

option(ENABLE_AVAHI "Enable Zeroconf" ${DEFAULT_AVAHI})
message(STATUS "ENABLE_AVAHI = " ${ENABLE_AVAHI})

option(ENABLE_USB_HID "Enable the libusb and hid devices" ${DEFAULT_USB_HID} )
message(STATUS "ENABLE_USB_HID = ${ENABLE_USB_HID}")

option(ENABLE_CEC "Enable the libcec and CEC control" ${DEFAULT_CEC} )
message(STATUS "ENABLE_CEC = ${ENABLE_CEC}")

option(ENABLE_X11 "Enable the X11 grabber" ${DEFAULT_X11})
message(STATUS "ENABLE_X11 = ${ENABLE_X11}")

option(ENABLE_XCB "Enable the XCB grabber" ${DEFAULT_XCB})
message(STATUS "ENABLE_XCB = ${ENABLE_XCB}")

option(ENABLE_QT "Enable the qt grabber" ${DEFAULT_QT})
message(STATUS "ENABLE_QT = ${ENABLE_QT}")

option(ENABLE_TESTS "Compile additional test applications" ${DEFAULT_TESTS})
message(STATUS "ENABLE_TESTS = ${ENABLE_TESTS}")

option(ENABLE_PROFILER "enable profiler capabilities - not for release code" OFF)
message(STATUS "ENABLE_PROFILER = ${ENABLE_PROFILER}")

option(ENABLE_EXPERIMENTAL "Compile experimental features" ${DEFAULT_EXPERIMENTAL})
message(STATUS "ENABLE_EXPERIMENTAL = ${ENABLE_EXPERIMENTAL}")

SET ( FLATBUFFERS_INSTALL_BIN_DIR ${CMAKE_BINARY_DIR}/flatbuf )
SET ( FLATBUFFERS_INSTALL_LIB_DIR ${CMAKE_BINARY_DIR}/flatbuf )

SET ( PROTOBUF_INSTALL_BIN_DIR ${CMAKE_BINARY_DIR}/proto )
SET ( PROTOBUF_INSTALL_LIB_DIR ${CMAKE_BINARY_DIR}/proto )

# check all json files
FILE ( GLOB_RECURSE HYPERION_SCHEMAS RELATIVE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/libsrc/*schema*.json )
SET( JSON_FILES
	config/hyperion.config.json.default
	${HYPERION_SCHEMAS}
)

EXECUTE_PROCESS (
	COMMAND ${PYTHON_EXECUTABLE} test/jsonchecks/checkjson.py ${JSON_FILES}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	RESULT_VARIABLE CHECK_JSON_FAILED
)
IF ( ${CHECK_JSON_FAILED} )
	MESSAGE (FATAL_ERROR "check of json files failed" )
ENDIF ()

EXECUTE_PROCESS (
	COMMAND ${PYTHON_EXECUTABLE} test/jsonchecks/checkeffects.py effects effects/schema
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	RESULT_VARIABLE CHECK_EFFECTS_FAILED
)
IF ( ${CHECK_EFFECTS_FAILED} )
	MESSAGE (FATAL_ERROR "check of json effect files failed" )
ENDIF ()

# for python 3 the checkschema.py file must be rewritten
# TODO on windows it can't resolve the path inside the file (Das System kann den angegebenen Pfad nicht finden: '\\schema\\schema-general.json')
IF (NOT WIN32)
	EXECUTE_PROCESS (
		COMMAND python test/jsonchecks/checkschema.py config/hyperion.config.json.default libsrc/hyperion/hyperion.schema.json
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		RESULT_VARIABLE CHECK_CONFIG_FAILED
	)
ENDIF()
IF ( ${CHECK_CONFIG_FAILED} )
	MESSAGE (FATAL_ERROR "check of json default config failed" )
ENDIF ()

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

# enable C++11; MSVC doesn't have c++11 feature switch
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
	CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	if (CMAKE_COMPILER_IS_GNUCXX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-psabi")
	endif()
	if(COMPILER_SUPPORTS_CXX11)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	elseif(COMPILER_SUPPORTS_CXX0X)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	else()
			message(STATUS "No support for C++11 detected. Compilation will most likely fail on your compiler")
	endif()
endif()

# MSVC options
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	# Search for Windows SDK
	find_package(WindowsSDK REQUIRED)
	message(STATUS "WINDOWS SDK: ${WINDOWSSDK_LATEST_DIR} ${WINDOWSSDK_LATEST_NAME}")
	message(STATUS "MSVC VERSION: ${MSVC_VERSION}")

	# Qt5 default install path with msvc2017 64bit component
	# The Qt5_DIR should point to Qt5Config.cmake   -> C:/Qt/5.xx/msvc2017_64/lib/cmake/Qt5
	# The CMAKE_PREFIX_PATH should point to the install directory -> C:/Qt/5.xx/msvc2017_64
	FIRSTSUBDIR(SUBDIRQT "C:/Qt")
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${SUBDIRQT}/msvc2019_64")
	if (NOT DEFINED ENV{Qt5_DIR})
		message(STATUS "Set Qt5 module path: ${SUBDIRQT}")
		SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${SUBDIRQT}/msvc2019_64/lib/cmake/Qt5")
	endif()
endif()

# Use GNU gold linker if available
if (NOT WIN32)
	include (${CMAKE_CURRENT_SOURCE_DIR}/cmake/LDGold.cmake)
endif()

# Don't create new dynamic tags (RUNPATH)
if (NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")
endif()

# setup -rpath to search for shared libs in BINARY/../lib folder
if (UNIX AND NOT APPLE)
	SET(CMAKE_SKIP_BUILD_RPATH FALSE)
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
	SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:$ORIGIN/../lib")
	SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif ()

# add QT5 dependency
IF ( CMAKE_CROSSCOMPILING )

	file(GLOB QT_BIN ${QT_BIN_PATH})

	set(QT_MOC_EXECUTABLE ${QT_BIN}/moc)
	add_executable(Qt5::moc IMPORTED)
	set_property(TARGET Qt5::moc PROPERTY IMPORTED_LOCATION ${QT_MOC_EXECUTABLE})

	set(QT_RCC_EXECUTABLE ${QT_BIN}/rcc)
	add_executable(Qt5::rcc IMPORTED)
	set_property(TARGET Qt5::rcc PROPERTY IMPORTED_LOCATION ${QT_RCC_EXECUTABLE})

	message(STATUS "QT_BIN_PATH = ${QT_BIN}")
	message(STATUS "QT_MOC_EXECUTABLE = ${QT_MOC_EXECUTABLE}")
	message(STATUS "QT_RCC_EXECUTABLE = ${QT_RCC_EXECUTABLE}")

ENDIF()

SET(QT_MIN_VERSION "5.5.0")
find_package(Qt5 COMPONENTS Core Gui Network SerialPort Sql REQUIRED)
message( STATUS "Found Qt Version: ${Qt5Core_VERSION}" )
IF ( "${Qt5Core_VERSION}" VERSION_LESS "${QT_MIN_VERSION}" )
	message( FATAL_ERROR "Your Qt version is to old! Minimum required ${QT_MIN_VERSION}" )
ENDIF()

# Add libusb and pthreads
find_package(libusb-1.0 REQUIRED)
find_package(Threads REQUIRED)
add_definitions(${QT_DEFINITIONS})

# Add JPEG library
if (ENABLE_V4L2)
	# Turbo JPEG
	find_package(TurboJPEG)
	if (TURBOJPEG_FOUND)
		add_definitions(-DHAVE_TURBO_JPEG)
		message( STATUS "Using Turbo JPEG library: ${TurboJPEG_LIBRARY}")
		include_directories(${TurboJPEG_INCLUDE_DIRS})
	else()
		# System JPEG
		find_package(JPEG)
		if (JPEG_FOUND)
			add_definitions(-DHAVE_JPEG)
			message( STATUS "Using system JPEG library: ${JPEG_LIBRARIES}")
			include_directories(${JPEG_INCLUDE_DIR})
		else()
			message( STATUS "JPEG library not found, MJPEG camera format won't work in V4L2 grabber.")
		endif()
	endif (TURBOJPEG_FOUND)

	if (TURBOJPEG_FOUND OR JPEG_FOUND)
		add_definitions(-DHAVE_JPEG_DECODER)
	endif()
endif()

if(APPLE)
	set(CMAKE_EXE_LINKER_FLAGS "-framework CoreGraphics")
endif()

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
	${CMAKE_BINARY_DIR}/EffectEngine.qrc
	${CMAKE_BINARY_DIR}/WebConfig.qrc
	${CMAKE_BINARY_DIR}/HyperionConfig.h
)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${GENERATED_QRC}" )

# uninstall target
configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

# enable make package - no code after this line !
include (${CMAKE_CURRENT_SOURCE_DIR}/cmake/packages.cmake)
