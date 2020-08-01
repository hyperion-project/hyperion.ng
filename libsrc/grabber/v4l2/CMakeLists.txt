# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/v4l2)

FILE ( GLOB V4L2_SOURCES "${CURRENT_HEADER_DIR}/V4L2*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(v4l2-grabber ${V4L2_SOURCES} )

target_link_libraries(v4l2-grabber
	hyperion
	${QT_LIBRARIES}
)

if(TURBOJPEG_FOUND)
	target_link_libraries(v4l2-grabber ${TurboJPEG_LIBRARY})
elseif (JPEG_FOUND)
	target_link_libraries(v4l2-grabber ${JPEG_LIBRARY})
endif(TURBOJPEG_FOUND)
