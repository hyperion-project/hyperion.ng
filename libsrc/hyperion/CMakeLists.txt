
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/hyperion)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/hyperion)

FILE ( GLOB Hyperion_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

SET(Hyperion_RESOURCES ${CURRENT_SOURCE_DIR}/resource.qrc)

add_library(hyperion
	${Hyperion_SOURCES}
	${Hyperion_RESOURCES}
)

target_link_libraries(hyperion
	blackborder
	hyperion-utils
	leddevice
	bonjour
	effectengine
	${QT_LIBRARIES}
)
