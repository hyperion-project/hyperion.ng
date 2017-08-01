
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/kodivideochecker)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/kodivideochecker)

SET(KODIVideoChecker_HEADERS
	${CURRENT_HEADER_DIR}/KODIVideoChecker.h
)

SET(KODIVideoChecker_SOURCES
	${CURRENT_SOURCE_DIR}/KODIVideoChecker.cpp
)


add_library(kodivideochecker
	${KODIVideoChecker_HEADERS}
	${KODIVideoChecker_SOURCES}
)

target_link_libraries(kodivideochecker
	hyperion
	${QT_LIBRARIES})
