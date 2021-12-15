
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/forwarder)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/forwarder)

if(ENABLE_FLATBUF_CONNECT)
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/flatbufserver
)
endif()

FILE ( GLOB Forwarder_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(forwarder ${Forwarder_SOURCES} )

target_link_libraries(forwarder
	hyperion
	hyperion-utils
	${QT_LIBRARIES}
)

if(ENABLE_FLATBUF_CONNECT)
	target_link_libraries(forwarder flatbufconnect)
endif()
