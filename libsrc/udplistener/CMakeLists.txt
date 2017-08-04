
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/udplistener)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/udplistener)

FILE ( GLOB UDPListener_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(udplistener ${UDPListener_SOURCES} )

target_link_libraries(udplistener
	hyperion
	hyperion-utils
	${QT_LIBRARIES}
)
