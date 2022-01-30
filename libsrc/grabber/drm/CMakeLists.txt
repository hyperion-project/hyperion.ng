# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/drm)

find_package(LibDRM 2.4.101 REQUIRED)

FILE ( GLOB DRMGrabberSOURCES "${CURRENT_HEADER_DIR}/DRM*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )
add_library( drm-grabber ${DRMGrabberSOURCES} )

target_include_directories(drm-grabber PUBLIC
	${LIBDRM_INCLUDE_DIRS}
)

target_link_libraries(drm-grabber
	hyperion
	${LIBDRM_LIBRARIES}
)
