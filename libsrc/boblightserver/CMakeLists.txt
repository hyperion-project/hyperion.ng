
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/boblightserver)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/boblightserver)

FILE ( GLOB BoblightServer_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(boblightserver ${BoblightServer_SOURCES} )

target_link_libraries(boblightserver
	hyperion
	hyperion-utils
	${QT_LIBRARIES}
)
