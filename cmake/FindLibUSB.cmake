# - Try to find libusb-1.0
# Once done this will define
#
#  LibUSB_FOUND - system has libusb
#  LibUSB_INCLUDE_DIR - the libusb include directory
#  LibUSB_LIBRARY - Link these to use libusb
#
#  Adapted from cmake-modules Google Code project
#
#  Copyright (c) 2006 Andreas Schneider <mail@cynapses.org>
#
#  (Changes for libusb) Copyright (c) 2008 Kyle Machulis <kyle@nonpolynomial.com>
#
# Redistribution and use is allowed according to the terms of the New BSD license.
#
# CMake-Modules Project New BSD License
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of the CMake-Modules Project nor the names of its
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

set(LIBUSB_ROOT_DIR "${LIBUSB_ROOT_DIR}" CACHE PATH "Root directory to search for libusb")

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
	pkg_check_modules(PC_LIBUSB QUIET libusb-1.0)
	if(NOT PC_LIBUSB_FOUND)
		pkg_check_modules(PC_LIBUSB QUIET libusb)
	endif()
	if(DEFINED PC_LIBUSB_VERSION AND NOT PC_LIBUSB_VERSION STREQUAL "")
		set(LibUSB_VERSION "${PC_LIBUSB_VERSION}")
	endif()
endif()

find_path(LibUSB_INCLUDE_DIR
	NAMES
		libusb.h usb.h
	PATHS
		/usr
		/usr/local
		/opt/local
		/opt/homebrew
		/sw
		${PC_LIBUSB_INCLUDE_DIRS}
		${CMAKE_BINARY_DIR}
	HINTS
		${LIBUSB_ROOT_DIR}
	PATH_SUFFIXES
		include
		libusb-1.0
		include/libusb-1.0
)

set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(LibUSB_LIBRARY
	NAMES
		libusb-1.0
		usb-1.0
		usb
	PATHS
		/usr
		/usr/local
		/opt/local
		/opt/homebrew
		/sw
		${PC_LIBUSB_LIBRARY_DIRS}
		${CMAKE_BINARY_DIR}
	HINTS
		${LIBUSB_ROOT_DIR}
	PATH_SUFFIXES
		lib
		VS2022/MS64/dll
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	LibUSB
	REQUIRED_VARS LibUSB_LIBRARY LibUSB_INCLUDE_DIR
	VERSION_VAR LibUSB_VERSION
)

mark_as_advanced(LibUSB_LIBRARY LibUSB_INCLUDE_DIR)

if(LibUSB_FOUND)
	if(NOT TARGET usb-1.0)
		add_library(usb-1.0 UNKNOWN IMPORTED GLOBAL)
		set_target_properties(usb-1.0 PROPERTIES
			IMPORTED_LINK_INTERFACE_LANGUAGES "C"
			IMPORTED_LOCATION "${LibUSB_LIBRARY}"
			INTERFACE_INCLUDE_DIRECTORIES "${LibUSB_INCLUDE_DIR}"
		)
	endif()

	if(CMAKE_SYSTEM_NAME MATCHES "Linux")
		get_target_property(TARGET_TYPE usb-1.0 TYPE)
		get_target_property(TARGET_LOCATION usb-1.0 LOCATION)
		get_filename_component(TARGET_EXTENSION ${TARGET_LOCATION} EXT)

		if((${TARGET_TYPE} STREQUAL "STATIC_LIBRARY") OR (${TARGET_EXTENSION} STREQUAL ${CMAKE_STATIC_LIBRARY_SUFFIX}))
			find_package(Libudev REQUIRED)
			set_target_properties(usb-1.0 PROPERTIES
				INTERFACE_LINK_LIBRARIES Libudev
			)
		endif()
	endif()

	if(NOT LibUSB_VERSION)
		# C code from: https://github.com/Nuand/bladeRF/blob/master/host/cmake/helpers/libusb_version.c
		file(WRITE ${CMAKE_BINARY_DIR}/tmp/src.c
			"#include <stdlib.h>
			#include <stdio.h>
			#include <string.h>
			#include <libusb.h>
			#if defined(WIN32)
				#include <winbase.h>
				typedef const struct libusb_version * (__stdcall * version_fn)(void);
			#endif
			int main(int argc, char *argv[])
			{
				const struct libusb_version *ver;
			#if defined(WIN32)
				HINSTANCE dll;
				version_fn get_version;
				dll = LoadLibraryA(argv[1]);
				get_version = (version_fn) GetProcAddress(dll, \"libusb_get_version\");
				if (get_version) {
					ver = get_version();
					printf(\"%u.%u.%u\", ver->major, ver->minor, ver->micro);
				}
				FreeLibrary(dll);
			#else
				ver = libusb_get_version();
				printf(\"%u.%u.%u\", ver->major, ver->minor, ver->micro);
			#endif
				return 0;
			}"
		)

		if(WIN32)
			string(REPLACE ".lib" ".dll" LIBUSB_DLL "${LIBUSB_LIBRARY}")
			try_run(RUN_RESULT COMPILE_RESULT
				${CMAKE_BINARY_DIR}
				${CMAKE_BINARY_DIR}/tmp/src.c
				CMAKE_FLAGS  -DINCLUDE_DIRECTORIES:STRING=${LIBUSB_INCLUDE_DIR}
				RUN_OUTPUT_VARIABLE OUTPUT_RESULT
				ARGS "\"${LIBUSB_DLL}\""
			)
		else()
			try_run(RUN_RESULT COMPILE_RESULT
				${CMAKE_BINARY_DIR}
				${CMAKE_BINARY_DIR}/tmp/src.c
				CMAKE_FLAGS -DINCLUDE_DIRECTORIES:STRING=${LIBUSB_INCLUDE_DIR} -DLINK_LIBRARIES:STRING=${LIBUSB_LIBRARY}
				RUN_OUTPUT_VARIABLE OUTPUT_RESULT
			)
		endif()

		if(RUN_RESULT EQUAL 0 AND COMPILE_RESULT)
			set(LibUSB_VERSION "${OUTPUT_RESULT}")
		endif()

		unset(RUN_RESULT)
		unset(COMPILE_RESULT)
	endif()
endif()
