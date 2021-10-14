# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/ssdp)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/ssdp)

FILE ( GLOB SSDP_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(ssdp
	${SSDP_SOURCES}
)

target_link_libraries(ssdp
	Qt${QT_VERSION_MAJOR}::Network
	webserver
)
