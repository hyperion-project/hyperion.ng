# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/v4l2)

SET(V4L2_QT_HEADERS
		${CURRENT_HEADER_DIR}/V4L2Grabber.h
)

SET(V4L2_HEADERS
		${CURRENT_HEADER_DIR}/VideoStandard.h
)

SET(V4L2_SOURCES
		${CURRENT_SOURCE_DIR}/V4L2Grabber.cpp
)

QT4_WRAP_CPP(V4L2_HEADERS_MOC ${V4L2_QT_HEADERS})

add_library(v4l2-grabber
		${V4L2_HEADERS}
		${V4L2_SOURCES}
		${V4L2_QT_HEADERS}
		${V4L2_HEADERS_MOC}
)

target_link_libraries(v4l2-grabber
	hyperion
	${QT_LIBRARIES}
)
