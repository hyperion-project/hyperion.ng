# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/v4l2)

SET(V4L2_HEADERS
	${CURRENT_HEADER_DIR}/V4L2Grabber.h
	${CURRENT_HEADER_DIR}/V4L2Wrapper.h
	${CURRENT_HEADER_DIR}/VideoStandard.h
)

SET(V4L2_SOURCES
	${CURRENT_SOURCE_DIR}/V4L2Grabber.cpp
	${CURRENT_SOURCE_DIR}/V4L2Wrapper.cpp
)

add_library(v4l2-grabber
	${V4L2_HEADERS}
	${V4L2_SOURCES}
)

target_link_libraries(v4l2-grabber
	hyperion
	${QT_LIBRARIES}
)
