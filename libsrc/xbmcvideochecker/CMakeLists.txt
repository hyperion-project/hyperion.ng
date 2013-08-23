
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/xbmcvideochecker)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/xbmcvideochecker)

# Group the headers that go through the MOC compiler
SET(XBMCVideoChecker_QT_HEADERS
		${CURRENT_HEADER_DIR}/XBMCVideoChecker.h
)

SET(XBMCVideoChecker_HEADERS
)

SET(XBMCVideoChecker_SOURCES
		${CURRENT_SOURCE_DIR}/XBMCVideoChecker.cpp
)

QT4_WRAP_CPP(XBMCVideoChecker_HEADERS_MOC ${XBMCVideoChecker_QT_HEADERS})

add_library(xbmcvideochecker
		${XBMCVideoChecker_HEADERS}
		${XBMCVideoChecker_QT_HEADERS}
		${XBMCVideoChecker_HEADERS_MOC}
		${XBMCVideoChecker_SOURCES}
)

target_link_libraries(xbmcvideochecker
		hyperion
		${QT_LIBRARIES})
