# - Try to find LibFtdi
#  Once done this will define
#
#  LIBFTDI_FOUND - System has LibFtdi
#  LIBFTDI_INCLUDE_DIR - The LibFtdi include directories
#  LIBFTDI_LIBRARY - The libraries needed to use LibFtdi
#

set(LIBFTDI1_ROOT_DIR "${LIBFTDI1_ROOT_DIR}" CACHE PATH "Root directory to search for LibFTDI1")

find_package(PkgConfig)
if (PkgConfig_FOUND)
	pkg_check_modules(PC_LIBFTDI1 QUIET libftdi1)
	if(DEFINED PC_LIBFTDI1_VERSION AND NOT PC_LIBFTDI1_VERSION STREQUAL "")
		set(LibFTDI1_VERSION "${PC_LIBFTDI1_VERSION}")
	endif()
endif()

find_path(LIBFTDI1_INCLUDE_DIR
	NAMES
		ftdi.h
	PATHS
		/usr
		/usr/local
		/opt/local
		/opt/homebrew
		/sw
		${PC_LIBFTDI1_INCLUDE_DIRS}
		${CMAKE_BINARY_DIR}
	HINTS
		${LIBFTDI1_ROOT_DIR}
	PATH_SUFFIXES
		include
		libftdi1
		include/libftdi1
)

set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(LIBFTDI1_LIBRARY
	NAMES
		ftdi1
		libftdi1
	PATHS
		/usr
		/usr/local
		/opt/local
		/opt/homebrew
		/sw
		${PC_LIBFTDI1_LIBRARIES}
		${CMAKE_BINARY_DIR}
	HINTS
		${LIBFTDI1_ROOT_DIR}
	PATH_SUFFIXES
		lib
		libftdi1
		lib/libftdi1
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	LibFTDI1
	REQUIRED_VARS LIBFTDI1_LIBRARY LIBFTDI1_INCLUDE_DIR
	VERSION_VAR LibFTDI1_VERSION
)

mark_as_advanced(LIBFTDI1_INCLUDE_DIR LIBFTDI1_LIBRARY)

if(LIBFTDI_FOUND)
	if(NOT TARGET libftdi)
		add_library(libftdi STATIC IMPORTED GLOBAL)
		set_target_properties(usb-1.0 PROPERTIES
			IMPORTED_LINK_INTERFACE_LANGUAGES "C"
			IMPORTED_LOCATION "${LIBFTDI1_LIBRARY}"
			INTERFACE_INCLUDE_DIRECTORIES "${LIBFTDI1_INCLUDE_DIR}"
		)
	endif()

	if(NOT LibFTDI1_VERSION)
		file(WRITE ${CMAKE_BINARY_DIR}/tmp/src.c
			"#include<ftdi.h>
			#include<stdio.h>
			int main()
			{
				printf(\"%s\",ftdi_get_library_version().version_str);
				return 0;
			}"
		)

		try_run(RUN_RESULT COMPILE_RESULT
			${CMAKE_BINARY_DIR}
			${CMAKE_BINARY_DIR}/tmp/src.c
			CMAKE_FLAGS -DINCLUDE_DIRECTORIES:STRING=${LIBFTDI1_INCLUDE_DIR} -DLINK_LIBRARIES:STRING=${LIBFTDI1_LIBRARY}
			RUN_OUTPUT_VARIABLE OUTPUT_RESULT
		)

		if(RUN_RESULT EQUAL 0 AND COMPILE_RESULT)
			set(LibFTDI1_VERSION "${OUTPUT_RESULT}")
		endif()

		unset(RUN_RESULT)
		unset(COMPILE_RESULT)
	endif()
endif()
