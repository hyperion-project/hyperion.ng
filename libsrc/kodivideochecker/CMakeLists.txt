
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/kodivideochecker)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/kodivideochecker)

# Group the headers that go through the MOC compiler
SET(KODIVideoChecker_QT_HEADERS
	${CURRENT_HEADER_DIR}/KODIVideoChecker.h
)

SET(KODIVideoChecker_HEADERS
)

SET(KODIVideoChecker_SOURCES
	${CURRENT_SOURCE_DIR}/KODIVideoChecker.cpp
)

QT5_WRAP_CPP(KODIVideoChecker_HEADERS_MOC ${KODIVideoChecker_QT_HEADERS})

add_library(kodivideochecker
	${KODIVideoChecker_HEADERS}
	${KODIVideoChecker_QT_HEADERS}
	${KODIVideoChecker_HEADERS_MOC}
	${KODIVideoChecker_SOURCES}
)

target_link_libraries(kodivideochecker
	hyperion
	${QT_LIBRARIES})
