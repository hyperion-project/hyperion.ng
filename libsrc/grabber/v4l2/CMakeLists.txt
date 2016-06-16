# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/v4l2)

SET(V4L2_QT_HEADERS
	${CURRENT_HEADER_DIR}/V4L2Grabber.h
	${CURRENT_HEADER_DIR}/V4L2Wrapper.h
)

SET(V4L2_HEADERS
	${CURRENT_HEADER_DIR}/VideoStandard.h
)

SET(V4L2_SOURCES
	${CURRENT_SOURCE_DIR}/V4L2Grabber.cpp
	${CURRENT_SOURCE_DIR}/V4L2Wrapper.cpp
)

QT5_WRAP_CPP(V4L2_HEADERS_MOC ${V4L2_QT_HEADERS})

add_library(v4l2-grabber
	${V4L2_HEADERS}
	${V4L2_SOURCES}
	${V4L2_QT_HEADERS}
	${V4L2_HEADERS_MOC}
)

qt5_use_modules(v4l2-grabber Widgets)

target_link_libraries(v4l2-grabber
	hyperion
	${QT_LIBRARIES}
)
