#  QMDNSENGINE_FOUND
#  QMDNS_INCLUDE_DIR
#  QMDNS_LIBRARIES

find_path(QMDNS_INCLUDE_DIR
    NAMES
        qmdnsengine/mdns.h
    PATH_SUFFIXES
        include
)

find_library(QMDNS_LIBRARIES
    NAMES
        libqmdnsengine.a
    PATHS
        /usr/local
        /usr
    PATH_SUFFIXES
        lib64
        lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(qmdnsengine
	FOUND_VAR qmdnsengine_FOUND
	REQUIRED_VARS QMDNS_INCLUDE_DIR QMDNS_LIBRARIES
)

if(qmdnsengine_FOUND AND NOT TARGET qmdnsengine)
    add_library(qmdnsengine STATIC IMPORTED)
    set_target_properties(qmdnsengine PROPERTIES
        IMPORTED_LOCATION ${QMDNS_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${QMDNS_INCLUDE_DIR}
    )
endif()
