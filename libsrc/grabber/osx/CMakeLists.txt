# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/osx)

FILE ( GLOB OsxGrabberSOURCES "${CURRENT_HEADER_DIR}/Osx*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(osx-grabber ${OsxGrabberSOURCES} )

target_link_libraries(osx-grabber
	hyperion
	${QT_LIBRARIES})
