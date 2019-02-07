
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/blackborder)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/blackborder)

FILE ( GLOB Blackborder_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(blackborder	${Blackborder_SOURCES} )

target_link_libraries(blackborder
 	hyperion-utils
	hyperion
)
