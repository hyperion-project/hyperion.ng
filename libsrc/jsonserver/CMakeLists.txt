
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/jsonserver)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/jsonserver)

FILE ( GLOB JsonServer_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(jsonserver ${JsonServer_SOURCES} )

target_link_libraries(jsonserver
	hyperion-api
	hyperion
	Qt${QT_VERSION_MAJOR}::Network
	Qt${QT_VERSION_MAJOR}::Gui
)

if(ENABLE_MDNS)
	target_link_libraries(jsonserver mdns)
endif()

