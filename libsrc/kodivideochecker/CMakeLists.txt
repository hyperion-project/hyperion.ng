
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/kodivideochecker)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/kodivideochecker)

FILE ( GLOB KODIVideoChecker_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(kodivideochecker ${KODIVideoChecker_SOURCES} )

target_link_libraries(kodivideochecker
	hyperion
	${QT_LIBRARIES})
