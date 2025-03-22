if(USE_PRE_BUILD_DEPS)
	# Set LIBUSB_ROOT_DIR
	set(LIBUSB_ROOT_DIR ${PRE_BUILD_DEPS_DIR})

	# Set paths for other dependencies
	foreach(BUILD_DEP "absl" "utf8_range" "hidapi" "qmdnsengine" "flatbuffers" "protobuf" "MbedTLS")
		set(${BUILD_DEP}_DIR "${PRE_BUILD_DEPS_DIR}/lib/cmake/${BUILD_DEP}")
	endforeach()
endif()

# LibFTDI or HIDAPI macOS configuration
if(CMAKE_SYSTEM_NAME MATCHES "Darwin" AND (ENABLE_DEV_USB_HID AND NOT USE_SYSTEM_HIDAPI_LIBS) OR (ENABLE_DEV_FTDI AND NOT USE_SYSTEM_LIBFTDI_LIBS))
	include(ExternalProject)
	set(APPLE_CONFIGURATION
		-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}
		-DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
		-DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
		-DCMAKE_MACOSX_RPATH:BOOL=${CMAKE_MACOSX_RPATH}
	)
endif()

# =============================================================================
# LibUSB (HIDAPI/LibFTDI need it)
# =============================================================================

if(ENABLE_DEV_USB_HID OR ENABLE_DEV_FTDI)
	if(USE_SYSTEM_LIBUSB_LIBS OR USE_PRE_BUILD_DEPS)
		find_package(LibUSB REQUIRED)
	endif()

	if(NOT USE_SYSTEM_LIBUSB_LIBS)
		ExternalProject_Add(libusb
			PREFIX				${CMAKE_CURRENT_BINARY_DIR}/external/libusb-cmake
			BUILD_ALWAYS		ON
			DOWNLOAD_COMMAND	""
			INSTALL_COMMAND		""
			SOURCE_DIR			${CMAKE_CURRENT_SOURCE_DIR}/external/libusb-cmake
			CMAKE_ARGS			-DBUILD_SHARED_LIBS:BOOL=OFF # Build libusb as static library
								-DLIBUSB_BUILD_TESTING:BOOL=OFF # Disable build of libusb tests
								-DLIBUSB_BUILD_EXAMPLES:BOOL=OFF # Disable build of libusb examples
								-DLIBUSB_INSTALL_TARGETS:BOOL=ON # Install libusb targets
								-DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
								-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
								-DCMAKE_WARN_DEPRECATED:BOOL=OFF # Suppress warnings about "Compatibility with CMake < 3.5 will be removed from a future version of CMake"
								${APPLE_CONFIGURATION}
			BUILD_BYPRODUCTS    <BINARY_DIR>/${CMAKE_STATIC_LIBRARY_PREFIX}libusb-1.0${CMAKE_STATIC_LIBRARY_SUFFIX}
		)

		add_library(usb-1.0 STATIC IMPORTED GLOBAL)
		add_dependencies(usb-1.0 libusb)
		ExternalProject_Get_Property(libusb BINARY_DIR SOURCE_DIR)
		set_target_properties(usb-1.0 PROPERTIES
			IMPORTED_LOCATION ${BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}libusb-1.0${CMAKE_STATIC_LIBRARY_SUFFIX}
			INTERFACE_INCLUDE_DIRECTORIES ${SOURCE_DIR}/libusb/libusb
		)

		if(CMAKE_SYSTEM_NAME MATCHES "Linux")
			find_package(Libudev REQUIRED)
			set_target_properties(usb-1.0 PROPERTIES
				INTERFACE_LINK_LIBRARIES Libudev
			)
		endif()
	endif()

	if(TARGET usb-1.0)
		# static libusb version detection
		get_build_interface_include_directory(TARGET usb-1.0 OUTPUT LIBUSB_INCLUDE_DIR)
		if(LIBUSB_INCLUDE_DIR AND EXISTS "${LIBUSB_INCLUDE_DIR}/version.h")
			file(READ "${LIBUSB_INCLUDE_DIR}/version.h" VERSIONHEADERDATA)
			string(REGEX MATCH "#define LIBUSB_MAJOR ([0-9]*)" _ ${VERSIONHEADERDATA})
			set(LIBUSB_VERSION_MAJOR ${CMAKE_MATCH_1})
			string(REGEX MATCH "#define LIBUSB_MINOR ([0-9]*)" _ ${VERSIONHEADERDATA})
			set(LIBUSB_VERSION_MINOR ${CMAKE_MATCH_1})
			string(REGEX MATCH "#define LIBUSB_MICRO ([0-9]*)" _ ${VERSIONHEADERDATA})
			set(LIBUSB_VERSION_MICRO ${CMAKE_MATCH_1})
			set(LibUSB_VERSION "${LIBUSB_VERSION_MAJOR}.${LIBUSB_VERSION_MINOR}.${LIBUSB_VERSION_MICRO}")
		endif()

		if(LibUSB_VERSION)
			message(STATUS "LibUSB version used: ${LibUSB_VERSION}")
		endif()
	endif()
endif()

# =============================================================================
# LibFTDI
# =============================================================================

if(ENABLE_DEV_FTDI)
	if(USE_SYSTEM_LIBFTDI_LIBS OR USE_PRE_BUILD_DEPS)
		find_package(LibFTDI1 REQUIRED)
	endif()

	if(NOT USE_SYSTEM_LIBFTDI_LIBS)
		# Pass LibUSB include directory and library location to LibFTDI
		get_build_interface_include_directory(TARGET usb-1.0 OUTPUT LIBUSB_INCLUDE_DIR)
		set(LIBUSB_INCLUDE_DIR ${LIBUSB_INCLUDE_DIR} CACHE INTERNAL "")
		get_target_property(LIBUSB_LIBRARIES usb-1.0 IMPORTED_LOCATION)
		set(LIBUSB_LIBRARIES ${LIBUSB_LIBRARIES} CACHE INTERNAL "")

		find_package(Git REQUIRED)
		ExternalProject_Add(libftdi1
			DEPENDS				usb-1.0
			PREFIX				${CMAKE_CURRENT_BINARY_DIR}/external/libftdi1
			GIT_REPOSITORY		git://developer.intra2net.com/libftdi
			GIT_TAG				de9f01ece34d2fe6e842e0250a38f4b16eda2429
			PATCH_COMMAND		git reset --hard HEAD && git apply ${CMAKE_CURRENT_SOURCE_DIR}/patch/libftdi/linkage.diff
			INSTALL_COMMAND		""
			CMAKE_ARGS			-DSTATICLIBS:BOOL=ON # Build LibFTDI as static library
								-DEXAMPLES:BOOL=OFF #  Disable build of LibFTDI example programs
								-DFTDI_EEPROM:BOOL=OFF # Disable building ftdi_eeprom
								-DLIBUSB_INCLUDE_DIR:PATH=${LIBUSB_INCLUDE_DIR} # LibUSB include directory
								-DLIBUSB_LIBRARIES:PATH=${LIBUSB_LIBRARIES} # LibUSB library location
								-DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
								-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
								-DCMAKE_WARN_DEPRECATED:BOOL=OFF # Suppress warnings about "Compatibility with CMake < 3.5 will be removed from a future version of CMake"
								${APPLE_CONFIGURATION}
			BUILD_BYPRODUCTS    <BINARY_DIR>/src/${CMAKE_STATIC_LIBRARY_PREFIX}ftdi1${CMAKE_STATIC_LIBRARY_SUFFIX}
		)

		add_library(libftdi STATIC IMPORTED GLOBAL)
		add_dependencies(libftdi libftdi1)
		ExternalProject_Get_Property(libftdi1 BINARY_DIR SOURCE_DIR)
		set_target_properties(libftdi PROPERTIES
			IMPORTED_LOCATION ${BINARY_DIR}/src/${CMAKE_STATIC_LIBRARY_PREFIX}ftdi1${CMAKE_STATIC_LIBRARY_SUFFIX}
			INTERFACE_INCLUDE_DIRECTORIES ${SOURCE_DIR}/src
		)

		# # Build LibFTDI as static library
		# set(STATICLIBS ON CACHE BOOL "Build statically version of LibFTDI")

		# # Disable build of LibFTDI example programs
		# set(EXAMPLES OFF CACHE BOOL "Build LibFTDI example programs")

		# # Disable building ftdi_eeprom
		# set(FTDI_EEPROM OFF CACHE BOOL "Build ftdi_eeprom")

		# if(WIN32)
		# 	# Patch LibFTDI because there is a conflict between shared/static build on windows
		# 	execute_process(COMMAND git apply "${CMAKE_CURRENT_SOURCE_DIR}/patch/libftdi/linkage.diff" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/libftdi)

		# 	# Set LibUSB include directory
		# 	get_build_interface_include_directory(TARGET usb-1.0 OUTPUT LIBUSB_INCLUDE_DIR)
		# 	set(LIBUSB_INCLUDE_DIR ${LIBUSB_INCLUDE_DIR} CACHE INTERNAL "")
		# 	# Set LibUSB library location
		# 	get_target_property(LIBUSB_LIBRARIES usb-1.0 IMPORTED_LOCATION)
		# 	set(LIBUSB_LIBRARIES ${LIBUSB_LIBRARIES} CACHE INTERNAL "")
		# endif()

		# # Add LibFTDI directory to the build
		# add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/libftdi")

		# Create INTERFACE target for ftdi1-static
		# if(TARGET ftdi1-static)
		# 	add_library(libftdi INTERFACE IMPORTED GLOBAL)
		# 	set_target_properties(libftdi PROPERTIES
		# 		INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/external/libftdi/src"
		# 		INTERFACE_LINK_LIBRARIES ftdi1-static
		# 	)
		# endif()
	endif()
endif()

# =============================================================================
# HIDAPI
# =============================================================================

if(ENABLE_DEV_USB_HID)
	if(USE_SYSTEM_HIDAPI_LIBS OR USE_PRE_BUILD_DEPS)
		find_package(hidapi CONFIG QUIET)
		if(NOT hidapi_FOUND)
			find_package(hidapi REQUIRED)
		endif()
	endif()

	if(NOT USE_SYSTEM_HIDAPI_LIBS)
		# Build HIDAPI as static library
		set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build statically version of HIDAPI")

		# Suppress warnings about "Compatibility with CMake < 3.5 will be removed from a future version of CMake"
		set(CMAKE_WARN_DEPRECATED OFF CACHE INTERNAL "")

		# Use the libusb based implementation of HIDAPI (Linux only)
		if(CMAKE_SYSTEM_NAME MATCHES "Linux")
			set(HIDAPI_WITH_HIDRAW OFF CACHE BOOL "Build HIDRAW-based implementation of HIDAPI")
		endif()

		# Install HIDAPI targets
		set(HIDAPI_INSTALL_TARGETS ON CACHE BOOL "Enable the installation of HIDAPI targets")

		# Disable build of HIDAPI tests
		set(HIDAPI_BUILD_HIDTEST OFF CACHE BOOL "Disable small console test application hidtest")

		# Add HIDAPI directory to the build
		add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/hidapi")
	endif()

	if(TARGET hidapi::hidapi AND NOT TARGET hidapi)
		add_library(hidapi INTERFACE IMPORTED GLOBAL)
		set_target_properties(hidapi PROPERTIES
			INTERFACE_LINK_LIBRARIES hidapi::hidapi
		)
	endif()

	if(hidapi_VERSION)
		message(STATUS "HIDAPI version used: ${hidapi_VERSION}")
	endif()
endif()

#=============================================================================
# Tinkerforge
#=============================================================================

if(ENABLE_DEV_TINKERFORGE)
	add_library(tinkerforge STATIC)
	target_sources(tinkerforge PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include/tinkerforge/bricklet_led_strip.h
		${CMAKE_CURRENT_SOURCE_DIR}/include/tinkerforge/ip_connection.h
		${CMAKE_CURRENT_SOURCE_DIR}/libsrc/tinkerforge/bricklet_led_strip.c
		${CMAKE_CURRENT_SOURCE_DIR}/libsrc/tinkerforge/ip_connection.c
	)

	target_include_directories(tinkerforge PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/include/tinkerforge"
	)
endif()

#=============================================================================
# rpi_ws281x
#=============================================================================

if(ENABLE_DEV_WS281XPWM)
	# Build rpi_ws281x as static library
	set(BUILD_SHARED OFF CACHE BOOL "Build statically version of rpi_ws281x")

	# Disable build of rpi_ws281x test application
	set(BUILD_TEST OFF CACHE BOOL "Build test application")

	# Add rpi_ws281x directory to the build
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/rpi_ws281x")
endif()

#=============================================================================
#  QMdnsEngine
#=============================================================================

if(ENABLE_MDNS)
	if(USE_SYSTEM_QMDNS_LIBS OR USE_PRE_BUILD_DEPS)
		find_package(qmdnsengine CONFIG QUIET)
		if(NOT qmdnsengine_FOUND)
			find_package(qmdnsengine REQUIRED)
		endif()
	endif()

	if(NOT USE_SYSTEM_QMDNS_LIBS)
		# Build QMdnsEngine as static library
		set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build statically version of QMdnsEngine")

		# Suppress warnings about "Compatibility with CMake < 3.5 will be removed from a future version of CMake"
		set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

		# Add QMdnsEngine directory to the build
		add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/qmdnsengine")
	endif()

	if(TARGET qmdnsengine)
		if(NOT TARGET qmdns)
			add_library(qmdns INTERFACE IMPORTED GLOBAL)
			target_link_libraries(qmdns INTERFACE qmdnsengine)
		endif()

		get_target_property(qmdnsengine_VERSION qmdnsengine VERSION)
		if(qmdnsengine_VERSION)
			message(STATUS "FlatBuffers version used: ${qmdnsengine_VERSION}")
		endif()
	endif()
endif()

#=============================================================================
# FlatBuffers
#=============================================================================

if(ENABLE_FLATBUF_SERVER OR ENABLE_FLATBUF_CONNECT)
	if(USE_SYSTEM_FLATBUFFERS_LIBS OR USE_PRE_BUILD_DEPS)
		find_package(flatbuffers CONFIG QUIET NAMES flatbuffers Flatbuffers FlatBuffers)
		if(NOT flatbuffers_FOUND)
			find_package(Flatbuffers REQUIRED)
		endif()

		if(NOT TARGET flatc)
			if(USE_PRE_BUILD_DEPS AND PRE_BUILD_DEPS_DIR)
				add_executable(flatc IMPORTED GLOBAL)
				set_target_properties(flatc PROPERTIES
					IMPORTED_LOCATION ${PRE_BUILD_DEPS_DIR}/bin/flatc${CMAKE_EXECUTABLE_SUFFIX}
				)
			elseif(TARGET flatbuffers::flatc)
				add_executable(flatc IMPORTED GLOBAL)
				get_target_property(FLATC_EXECUTABLE flatbuffers::flatc IMPORTED_LOCATION_RELEASE)
				set_target_properties(flatc PROPERTIES IMPORTED_LOCATION ${FLATC_EXECUTABLE})
			endif()
		endif()
	endif()

	if(NOT USE_SYSTEM_FLATBUFFERS_LIBS)
		# Build Flatbuffers as static library
		set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared Flatbuffers library")

		# Disable build of Flatbuffers tests
		set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "Build Flatbuffers with tests")

		# Define the flatc import option
		set(IMPORT_FLATC "" CACHE STRING "Import the Flatbuffers compiler (flatc_export.cmake) from a native build")

		# Add Flatbuffers directory to the build
		add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/flatbuffers")

		if(CMAKE_CROSSCOMPILING)
			# Disable Flatbuffers Compiler (import or build nativly)
			set(FLATBUFFERS_BUILD_FLATC OFF CACHE BOOL "Enable the build of the flatbuffers compiler")

			if(IMPORT_FLATC)
				# Import the Flatbuffers Compiler from a native build ...
				include(${IMPORT_FLATC})
			else()
				# ... or build flatc nativly
				include(ExternalProject)
				ExternalProject_Add(flatc-host
					PREFIX				${CMAKE_BINARY_DIR}/dependencies/external/flatc-host
					BUILD_ALWAYS		OFF
					DOWNLOAD_COMMAND	""
					INSTALL_COMMAND     ""
					SOURCE_DIR			${CMAKE_CURRENT_SOURCE_DIR}/external/flatbuffers
					CMAKE_ARGS          -DFLATBUFFERS_BUILD_FLATLIB:BOOL=OFF
										-DFLATBUFFERS_INSTALL:BOOL=OFF
										-DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
										-DFLATBUFFERS_BUILD_TESTS:BOOL=${FLATBUFFERS_BUILD_TESTS}
										-DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
										-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
										-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
										-Wno-dev # We don't want to be warned over unused variables
					BUILD_BYPRODUCTS    <BINARY_DIR>/flatc${CMAKE_EXECUTABLE_SUFFIX}
				)

				add_executable(flatc IMPORTED GOBAL)
				ExternalProject_Get_Property(flatc-host BINARY_DIR)
				set_target_properties(flatc PROPERTIES
					IMPORTED_LOCATION ${BINARY_DIR}/flatc${CMAKE_EXECUTABLE_SUFFIX}
				)
				add_dependencies(flatc flatc-host)
			endif()
		else()
			# export the flatc compiler so it can be used when cross compiling
			export(TARGETS flatc FILE "${CMAKE_BINARY_DIR}/flatc_export.cmake")
		endif()
	endif()

	if(TARGET flatbuffers::flatbuffers OR TARGET FlatBuffers::FlatBuffers)
		if(NOT TARGET FlatBuffers)
			add_library(FlatBuffers INTERFACE IMPORTED GLOBAL)
			if(TARGET flatbuffers::flatbuffers)
				target_link_libraries(FlatBuffers INTERFACE flatbuffers::flatbuffers)
				get_build_interface_include_directory(TARGET flatbuffers::flatbuffers OUTPUT FLATBUFFERS_INCLUDE_DIR)
			else()
				target_link_libraries(FlatBuffers INTERFACE FlatBuffers::FlatBuffers)
				get_build_interface_include_directory(TARGET FlatBuffers::FlatBuffers OUTPUT FLATBUFFERS_INCLUDE_DIR)
			endif()
		endif()

		# static flatbuffers version detection from: https://github.com/hyperledger-archives/iroha-ametsuchi/blob/master/cmake/Modules/Findflatbuffers.cmake
		if(FLATBUFFERS_INCLUDE_DIR AND EXISTS "${FLATBUFFERS_INCLUDE_DIR}/flatbuffers/base.h")
			file(STRINGS "${FLATBUFFERS_INCLUDE_DIR}/flatbuffers/base.h" flatbuffers_version_str REGEX "#define FLATBUFFERS_VERSION_[A-Z]+ ")
			string(REGEX REPLACE ".*#define FLATBUFFERS_VERSION_MAJOR ([0-9]+).*" "\\1" flatbuffers_MAJOR_VERSION "${flatbuffers_version_str}")
			string(REGEX REPLACE ".*#define FLATBUFFERS_VERSION_MINOR ([0-9]+).*" "\\1" flatbuffers_MINOR_VERSION "${flatbuffers_version_str}")
			string(REGEX REPLACE ".*#define FLATBUFFERS_VERSION_REVISION ([0-9]+).*" "\\1" flatbuffers_REVISION_VERSION "${flatbuffers_version_str}")
			message(STATUS "FlatBuffers version used: ${flatbuffers_MAJOR_VERSION}.${flatbuffers_MINOR_VERSION}.${flatbuffers_REVISION_VERSION}")
		endif()
	endif()

	function(compile_flatbuffer_schema FBS_GENERATED)
		if(NOT ARGN)
			message(SEND_ERROR "Error: compile_flatbuffer_schema() called without any schema files")
			return()
		endif()

		set(${FBS_GENERATED})
		foreach(FIL ${ARGN})
			get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
			get_filename_component(FIL_WE ${FIL} NAME_WE)

			set(OUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}_generated.h")
			list(APPEND ${FBS_GENERATED} ${OUT_FILE})

			add_custom_command(
				OUTPUT ${OUT_FILE}
				COMMAND "$<TARGET_FILE:flatc>"
				ARGS -c --no-includes --gen-mutable --gen-object-api -o "${CMAKE_CURRENT_BINARY_DIR}" "${ABS_FIL}"
				DEPENDS ${ABS_FIL} flatc
				COMMENT "Running flatbuffers compiler on ${FIL}"
				VERBATIM
			)
			set_property(SOURCE ${OUT_FILE} PROPERTY SKIP_AUTOMOC ON)
		endforeach()

		set_source_files_properties(${${FBS_GENERATED}} PROPERTIES
			GENERATED TRUE
		)
		set(${FBS_GENERATED} ${${FBS_GENERATED}} PARENT_SCOPE)
	endfunction()
endif()

#=============================================================================
# Protocol Buffers
#=============================================================================

if(ENABLE_PROTOBUF_SERVER)
	if(USE_SYSTEM_PROTO_LIBS OR USE_PRE_BUILD_DEPS)
		find_package(protobuf CONFIG QUIET NAMES protobuf Protobuf ProtoBuf)
		if(NOT protobuf_FOUND)
			find_package(Protobuf REQUIRED)
		endif()

		if(NOT TARGET protoc)
			if(USE_PRE_BUILD_DEPS AND PRE_BUILD_DEPS_DIR)
				add_executable(protoc IMPORTED GLOBAL)
				set_target_properties(protoc PROPERTIES
					IMPORTED_LOCATION ${PRE_BUILD_DEPS_DIR}/bin/protoc${CMAKE_EXECUTABLE_SUFFIX}
				)
			elseif(TARGET protobuf::protoc)
				add_executable(protoc IMPORTED GLOBAL)
				get_target_property(PROTOC_EXECUTABLE protobuf::protoc IMPORTED_LOCATION_RELEASE)
				set_target_properties(protoc PROPERTIES IMPORTED_LOCATION ${PROTOC_EXECUTABLE})
			endif()
		endif()
	endif()

	if(NOT USE_SYSTEM_PROTO_LIBS)
		# Build Protobuf as static library
		set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "Build protobuf shared")

		# Disable build of Protobuf tests
		set(protobuf_BUILD_TESTS OFF CACHE BOOL "Build protobuf with tests")

		# Disable build of Protobuf with zlib support
		set(protobuf_WITH_ZLIB OFF CACHE BOOL "Build protobuf with zlib support")

		# Disable build of libupb
		set(protobuf_BUILD_LIBUPB OFF CACHE BOOL "Build libupb")

		# Build abeil (3rd party sub-module) with C++ version requirements
		set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "Build abseil-cpp with C++ version requirements propagated")

		# Override debug prefix
		set(protobuf_DEBUG_POSTFIX "" CACHE STRING "Default debug postfix" FORCE)

		# Disable static linking of MSVC runtime libraries under Windows
		if(WIN32)
			set(protobuf_MSVC_STATIC_RUNTIME OFF CACHE BOOL "Build protobuf static")
			if(USE_PRE_BUILD_DEPS)
				set(CMAKE_INSTALL_CMAKEDIR "lib/cmake/protobuf")
			endif()
		endif()

		# Define the protoc import option
		set(IMPORT_PROTOC "" CACHE STRING "Import the Protobuf compiler (protoc_export.cmake) from a native build")

		# Add Protobuf directory to the build
		add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/protobuf")

		if(CMAKE_CROSSCOMPILING)
			# Disable Protobuf Compiler (import or build nativly)
			set(protobuf_BUILD_PROTOC_BINARIES OFF CACHE BOOL "Build protobuf libraries and protoc compiler")

			if(IMPORT_PROTOC)
				# Import the Protobuf Compiler from a native build ...
				include(${IMPORT_FLATC})
			else()
				# ... or build protoc nativly
				include(ExternalProject)
				ExternalProject_Add(protoc-host
					PREFIX				${CMAKE_BINARY_DIR}/dependencies/external/protoc-host
					BUILD_ALWAYS		OFF
					DOWNLOAD_COMMAND	""
					INSTALL_COMMAND     ""
					SOURCE_DIR			${CMAKE_CURRENT_SOURCE_DIR}/external/protobuf
					CMAKE_ARGS          -Dprotobuf_BUILD_LIBPROTOC:BOOL=OFF
										-Dprotobuf_INSTALL:BOOL=OFF
										-Dprotobuf_BUILD_TESTS:BOOL=${protobuf_BUILD_TESTS}
										-Dprotobuf_BUILD_SHARED_LIBS:BOOL=${protobuf_BUILD_SHARED_LIBS}
										-Dprotobuf_WITH_ZLIB:BOOL=${protobuf_WITH_ZLIB}
										-DABSL_PROPAGATE_CXX_STD:BOOL=${ABSL_PROPAGATE_CXX_STD}
										-DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
										-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
										-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
										-Wno-dev # We don't want to be warned over unused variables
					BUILD_BYPRODUCTS    <BINARY_DIR>/protoc${CMAKE_EXECUTABLE_SUFFIX}
				)

				add_executable(protoc IMPORTED GLOBAL)
				ExternalProject_Get_Property(protoc-host BINARY_DIR)
				set_target_properties(protoc PROPERTIES
					IMPORTED_LOCATION ${BINARY_DIR}/protoc${CMAKE_EXECUTABLE_SUFFIX}
				)
				add_dependencies(protoc protoc-host)
			endif()
		else()
			# export the protoc compiler so it can be used when cross compiling
			export(TARGETS protoc FILE "${CMAKE_BINARY_DIR}/protoc_export.cmake")
		endif()
	endif()

	if(TARGET protobuf::libprotobuf)
		if(NOT TARGET ProtoBuf)
			add_library(ProtoBuf INTERFACE IMPORTED GLOBAL)
			target_link_libraries(ProtoBuf INTERFACE protobuf::libprotobuf)
		endif()

		# static protobuf version detection from: https://github.com/opencv/opencv/blob/4.x/cmake/OpenCVFindProtobuf.cmake
		get_build_interface_include_directory(TARGET protobuf::libprotobuf OUTPUT PROTOBUF_INCLUDE_DIR)
		if(PROTOBUF_INCLUDE_DIR AND EXISTS "${PROTOBUF_INCLUDE_DIR}/google/protobuf/stubs/common.h")
			file(STRINGS "${PROTOBUF_INCLUDE_DIR}/google/protobuf/stubs/common.h" ver REGEX "#define GOOGLE_PROTOBUF_VERSION [0-9]+")
			string(REGEX MATCHALL "[0-9]+" ver ${ver})
			math(EXPR minor "${ver} / 1000 % 1000")
			math(EXPR patch "${ver} % 1000")
			message(STATUS "Protocol Buffers version used: ${minor}.${patch}")
		endif()
	endif()

	function(protobuf_generate_cpp SRCS HDRS)
		if(NOT ARGN)
			message(SEND_ERROR "Error: PROTOBUF_GENERATE_CPP() called without any proto files")
			return()
		endif()

		if(PROTOBUF_GENERATE_CPP_APPEND_PATH)
			# Create an include path for each file specified
			foreach(FIL ${ARGN})
				get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
				get_filename_component(ABS_PATH ${ABS_FIL} PATH)
				list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
				if(${_contains_already} EQUAL -1)
					list(APPEND _protobuf_include_path -I ${ABS_PATH})
				endif()
			endforeach()
		else()
			set(_protobuf_include_path -I ${CMAKE_CURRENT_SOURCE_DIR})
		endif()

		if(DEFINED PROTOBUF_IMPORT_DIRS)
			foreach(DIR ${PROTOBUF_IMPORT_DIRS})
				get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
				list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
				if(${_contains_already} EQUAL -1)
					list(APPEND _protobuf_include_path -I ${ABS_PATH})
				endif()
			endforeach()
		endif()

		set(${SRCS})
		set(${HDRS})
		foreach(FIL ${ARGN})
			get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
			get_filename_component(FIL_WE ${FIL} NAME_WE)

			list(APPEND ${SRCS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc")
			list(APPEND ${HDRS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h")

			add_custom_command(
				OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc"
						"${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h"
				COMMAND "$<TARGET_FILE:protoc>"
				ARGS --cpp_out ${CMAKE_CURRENT_BINARY_DIR} ${_protobuf_include_path} ${ABS_FIL}
				DEPENDS ${ABS_FIL} protoc
				COMMENT "Running C++ protocol buffer compiler on ${FIL}"
				VERBATIM
			)
			set_property(SOURCE "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc" PROPERTY SKIP_AUTOMOC ON)
			set_property(SOURCE "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h" PROPERTY SKIP_AUTOMOC ON)
		endforeach()

		# disable warnings for auto generated proto files, we can't change the files ....
		if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
			set_source_files_properties(${${SRCS}} ${${HDRS}} ${ARGN} PROPERTIES
				COMPILE_FLAGS "-w -Wno-return-local-addr"
			)
		elseif(MSVC)
			set_source_files_properties(${${SRCS}} ${${HDRS}} ${ARGN} PROPERTIES
				COMPILE_FLAGS "/W0"
			)
		endif()

		set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES
			GENERATED TRUE
		)
		set(${SRCS} ${${SRCS}} PARENT_SCOPE)
		set(${HDRS} ${${HDRS}} PARENT_SCOPE)
	endfunction()
endif()

#=============================================================================
# MbedTLS
#=============================================================================

if(ENABLE_DEV_NETWORK)
	if(USE_SYSTEM_MBEDTLS_LIBS OR USE_PRE_BUILD_DEPS)
		find_package(MbedTLS CONFIG QUIET NAMES mbedtls mbedTLS MbedTLS)
		if(NOT MbedTLS_FOUND)
			find_package(MbedTLS REQUIRED)
		endif()
	endif()

	if(NOT USE_SYSTEM_MBEDTLS_LIBS)
		# Build MbedTLS as static library
		set(USE_SHARED_MBEDTLS_LIBRARY OFF CACHE BOOL "Disable MbedTLS shared libraries")
		set(USE_STATIC_MBEDTLS_LIBRARY ON CACHE BOOL "Enable MbedTLS static libraries")

		# Disable build of MbedTLS tests
		set(ENABLE_TESTING OFF CACHE BOOL "Build MbedTLS tests")

		# Disable fatal warnings
		option(MBEDTLS_FATAL_WARNINGS "Compiler warnings treated as errors" OFF)

		# Disable build of MbedTLS program building
		set(ENABLE_PROGRAMS OFF CACHE BOOL "Build MbedTLS programs")

		# Enable export
		set(DISABLE_PACKAGE_CONFIG_AND_INSTALL OFF CACHE INTERNAL "Disable package configuration, target export and installation")

		# Add MbedTLS directory to the build
		add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/mbedtls")
	endif()

	if(TARGET MbedTLS::mbedtls)
		if(NOT TARGET MbedTLS)
			add_library(MbedTLS INTERFACE IMPORTED GLOBAL)
			target_link_libraries(MbedTLS INTERFACE MbedTLS::mbedtls)
		endif()

		# static mbedtls version detection
		get_build_interface_include_directory(TARGET MbedTLS::mbedtls OUTPUT MBEDTLS_INCLUDE_DIR)
		if(EXISTS "${MBEDTLS_INCLUDE_DIR}/mbedtls/build_info.h")
			file(STRINGS ${MBEDTLS_INCLUDE_DIR}/mbedtls/build_info.h _MBEDTLS_VERSION_LINE REGEX "^#define[ \t]+MBEDTLS_VERSION_STRING[\t ].*")
			string(REGEX REPLACE ".*MBEDTLS_VERSION_STRING[\t ]+\"(.*)\"" "\\1" MbedTLS_VERSION ${_MBEDTLS_VERSION_LINE})
		elseif(EXISTS "${MBEDTLS_INCLUDE_DIR}/mbedtls/version.h")
			file(STRINGS "${MBEDTLS_INCLUDE_DIR}/mbedtls/version.h" _MBEDTLS_VERSION_STRING REGEX "^#[\t ]*define[\t ]+MBEDTLS_VERSION_STRING[\t ]+\"[0-9]+.[0-9]+.[0-9]+\"")
			string(REGEX REPLACE "^.*MBEDTLS_VERSION_STRING.*([0-9]+.[0-9]+.[0-9]+).*" "\\1" MbedTLS_VERSION "${_MBEDTLS_VERSION_STRING}")
		endif()

		if(MbedTLS_VERSION)
			message(STATUS "MbedTLS version used: ${MbedTLS_VERSION}")

			# Define and set custom INTERFACE_MBEDTLS_VERSION_PROPERTY and INTERFACE_MBEDTLS_MAJOR_VERSION_PROPERTY target property
			string(REGEX MATCH "[0-9]+|-([A-Za-z0-9_.]+)" MBEDTLS_MAJOR_VERSION ${MbedTLS_VERSION})
			define_property(TARGET PROPERTY INTERFACE_MBEDTLS_VERSION_PROPERTY INHERITED
				BRIEF_DOCS "Custom MbedTLS version target property."
				FULL_DOCS "Custom MbedTLS version target property."
			)
			define_property(TARGET PROPERTY INTERFACE_MBEDTLS_MAJOR_VERSION_PROPERTY INHERITED
				BRIEF_DOCS "Custom MbedTLS major version target property."
				FULL_DOCS "Custom MbedTLS major version target property."
			)

			set_target_properties(MbedTLS PROPERTIES
				INTERFACE_MBEDTLS_VERSION_PROPERTY ${MbedTLS_VERSION}
				INTERFACE_MBEDTLS_MAJOR_VERSION_PROPERTY ${MBEDTLS_MAJOR_VERSION}
			)
		endif()
	endif()
endif(ENABLE_DEV_NETWORK)
