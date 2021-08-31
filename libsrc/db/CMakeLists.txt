# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/db)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/db)

FILE ( GLOB DB_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(database
	${DB_SOURCES}
)

target_link_libraries(database
	hyperion
	hyperion-utils
	Qt${Qt_VERSION}::Core
	Qt${Qt_VERSION}::Sql
)
