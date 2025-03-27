# Copyright (C) 2001-2024 Mixxx Development Team
# Distributed under the GNU General Public Licence (GPL) version 2 or any later
# later version. See the LICENSE file for details.
#
# Modified by Hyperion Project
#
#[=======================================================================[.rst:
FindLibudev
--------

Finds the libudev (userspace device manager) library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``Libudev``
  The libudev library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``Libudev_FOUND``
  True if the system has the libudev library.

#]=======================================================================]

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
	message(STATUS "PkgConfig found!!!")
	pkg_check_modules(PC_Libudev QUIET libudev)
endif()

find_path(Libudev_INCLUDE_DIR
	NAMES
		libudev.h
	PATHS
		/usr
		/usr/local
		${PC_Libudev_INCLUDE_DIRS}
	PATH_SUFFIXES
		include
)

find_library(Libudev_LIBRARY
	NAMES
		udev
		libudev
	PATHS
		/usr
		/usr/local
		${PC_Libudev_LIBRARY_DIRS}
	PATH_SUFFIXES
		lib
)

message(STATUS "Libudev LIBRARY: ${Libudev_LIBRARY}")
message(STATUS "Libudev INCLUDE_DIR: ${Libudev_INCLUDE_DIR}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	Libudev
	REQUIRED_VARS Libudev_LIBRARY Libudev_INCLUDE_DIR
	VERSION_VAR Libudev_VERSION
)

mark_as_advanced(Libudev_INCLUDE_DIR Libudev_LIBRARY)

if(Libudev_FOUND)
	if(NOT TARGET Libudev)
		add_library(Libudev UNKNOWN IMPORTED)
		set_target_properties(Libudev PROPERTIES
			IMPORTED_LOCATION "${Libudev_LIBRARY}"
			INTERFACE_INCLUDE_DIRECTORIES "${Libudev_INCLUDE_DIR}"
		)
	endif()
endif ()
