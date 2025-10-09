find_package(LibDRM 2.4.101 REQUIRED)

add_library(drm-grabber
	${CMAKE_SOURCE_DIR}/include/grabber/drm/DRMFrameGrabber.h
	${CMAKE_SOURCE_DIR}/include/grabber/drm/DRMWrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/drm/DRMFrameGrabber.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/drm/DRMWrapper.cpp
)

target_include_directories(drm-grabber
	PUBLIC
	${LIBDRM_INCLUDE_DIRS}
)

target_link_libraries(drm-grabber
	hyperion
	${LIBDRM_LIBRARIES}
)

