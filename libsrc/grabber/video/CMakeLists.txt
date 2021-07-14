# Common cmake definition for external video grabber

# Add Turbo JPEG library
if (ENABLE_V4L2 OR ENABLE_MF)
	find_package(TurboJPEG)
	if (TURBOJPEG_FOUND)
		add_definitions(-DHAVE_TURBO_JPEG)
		message( STATUS "Using Turbo JPEG library: ${TurboJPEG_LIBRARY}")
		include_directories(${TurboJPEG_INCLUDE_DIRS})
	else ()
		message( STATUS "Turbo JPEG library not found, MJPEG camera format won't work.")
	endif ()
endif()

# Define the wrapper/header/source locations and collect them
SET(WRAPPER_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/video)
SET(HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
if (ENABLE_MF)
	project(mf-grabber)
	SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/video/mediafoundation)
	FILE (GLOB SOURCES "${WRAPPER_DIR}/*.cpp" "${HEADER_DIR}/Video*.h" "${HEADER_DIR}/MF*.h" "${HEADER_DIR}/Encoder*.h" "${CURRENT_SOURCE_DIR}/*.h" "${CURRENT_SOURCE_DIR}/*.cpp")
elseif(ENABLE_V4L2)
	project(v4l2-grabber)
	SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/video/v4l2)
	FILE (GLOB SOURCES "${WRAPPER_DIR}/*.cpp" "${HEADER_DIR}/Video*.h" "${HEADER_DIR}/V4L2*.h" "${HEADER_DIR}/Encoder*.h" "${CURRENT_SOURCE_DIR}/*.cpp")
endif()

add_library(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} hyperion ${QT_LIBRARIES})

if(TURBOJPEG_FOUND)
	target_link_libraries(${PROJECT_NAME} ${TurboJPEG_LIBRARY})
endif()
