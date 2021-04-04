# Common cmake definition for external video grabber

# Define the wrapper/header/source locations and collect them
SET(WRAPPER_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/video)
SET(HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
if (ENABLE_MF)
	project(mf-grabber)
	SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/video/mediafoundation)
	FILE (GLOB SOURCES "${WRAPPER_DIR}/*.cpp" "${HEADER_DIR}/Video*.h" "${HEADER_DIR}/MF*.h" "${CURRENT_SOURCE_DIR}/*.h" "${CURRENT_SOURCE_DIR}/*.cpp")
elseif(ENABLE_V4L2)
	project(v4l2-grabber)
	SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/video/v4l2)
	FILE (GLOB SOURCES "${WRAPPER_DIR}/*.cpp" "${HEADER_DIR}/Video*.h" "${HEADER_DIR}/V4L2*.h" "${CURRENT_SOURCE_DIR}/*.h" "${CURRENT_SOURCE_DIR}/*.cpp")
endif()

add_library(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} hyperion ${QT_LIBRARIES})

if(TURBOJPEG_FOUND)
	target_link_libraries(${PROJECT_NAME} ${TurboJPEG_LIBRARY})
elseif (JPEG_FOUND)
	target_link_libraries(${PROJECT_NAME} ${JPEG_LIBRARY})
endif()
