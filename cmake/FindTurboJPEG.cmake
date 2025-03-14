#  FindTurboJPEG.cmake
#  TURBOJPEG_FOUND
#  TurboJPEG_INCLUDE_DIR
#  TurboJPEG_LIBRARY

find_path(TurboJPEG_INCLUDE_DIR
	NAMES
		turbojpeg.h
	PATHS
		"C:/libjpeg-turbo64"
	PATH_SUFFIXES
		include
)

find_library(TurboJPEG_LIBRARY
	NAMES
		libturbojpeg
		turbojpeg
		turbojpeg-static
	PATHS
		"C:/libjpeg-turbo64"
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
