
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/jsonserver)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/jsonserver)

FILE ( GLOB JsonServer_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(jsonserver ${JsonServer_SOURCES} )

target_link_libraries(jsonserver
	hyperion-api
	hyperion
	Qt5::Network
	Qt5::Gui
)
