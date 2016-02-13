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

if(ENABLE_QT5)
QT5_WRAP_CPP(V4L2_HEADERS_MOC ${V4L2_QT_HEADERS})
else(ENABLE_QT5)
QT4_WRAP_CPP(V4L2_HEADERS_MOC ${V4L2_QT_HEADERS})
endif(ENABLE_QT5)

add_library(v4l2-grabber
		${V4L2_HEADERS}
		${V4L2_SOURCES}
		${V4L2_QT_HEADERS}
		${V4L2_HEADERS_MOC}
)

if(ENABLE_QT5)
qt5_use_modules(v4l2-grabber Widgets)
endif(ENABLE_QT5)

target_link_libraries(v4l2-grabber
	hyperion
	${QT_LIBRARIES}
)
