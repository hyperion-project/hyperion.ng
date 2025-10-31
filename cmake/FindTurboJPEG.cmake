#  FindTurboJPEG.cmake
#  TURBOJPEG_FOUND
#  TurboJPEG_INCLUDE_DIR
#  TurboJPEG_LIBRARY

set(TURBOJPEG_ROOT_DIR "${TURBOJPEG_ROOT_DIR}" CACHE PATH "Root directory to search for TurboJPEG")

find_path(TurboJPEG_INCLUDE_DIR
	NAMES
		turbojpeg.h
	PATHS
		"C:/libjpeg-turbo64"
	HINTS
		${TURBOJPEG_ROOT_DIR}
	PATH_SUFFIXES
		include
)

find_library(TurboJPEG_LIBRARY
	NAMES
		turbojpeg-static
		turbojpeg
		libturbojpeg-static
		libturbojpeg
	PATHS
		"C:/libjpeg-turbo64"
	HINTS
		${TURBOJPEG_ROOT_DIR}
	PATH_SUFFIXES
		bin
		lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TurboJPEG
	FOUND_VAR
		TurboJPEG_FOUND
	REQUIRED_VARS
		TurboJPEG_LIBRARY
		TurboJPEG_INCLUDE_DIR
)

if(TurboJPEG_FOUND AND NOT TARGET turbojpeg)
	add_library(turbojpeg UNKNOWN IMPORTED GLOBAL)
	set_target_properties(turbojpeg PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${TurboJPEG_INCLUDE_DIR}"
		IMPORTED_LOCATION "${TurboJPEG_LIBRARY}"
	)
endif()
