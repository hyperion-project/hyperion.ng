
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

if(ENABLE_QT5)
QT5_WRAP_CPP(XBMCVideoChecker_HEADERS_MOC ${XBMCVideoChecker_QT_HEADERS})
else(ENABLE_QT5)
QT4_WRAP_CPP(XBMCVideoChecker_HEADERS_MOC ${XBMCVideoChecker_QT_HEADERS})
endif(ENABLE_QT5)

add_library(xbmcvideochecker
		${XBMCVideoChecker_HEADERS}
		${XBMCVideoChecker_QT_HEADERS}
		${XBMCVideoChecker_HEADERS_MOC}
		${XBMCVideoChecker_SOURCES}
)

if(ENABLE_QT5)
qt5_use_modules(xbmcvideochecker Widgets)
endif(ENABLE_QT5)

target_link_libraries(xbmcvideochecker
		hyperion
		${QT_LIBRARIES})
