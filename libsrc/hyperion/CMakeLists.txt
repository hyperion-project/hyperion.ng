
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/hyperion)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/hyperion)

if(ENABLE_FLATBUF_SERVER)
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/flatbufserver
)
endif()

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
	database
	${QT_LIBRARIES}
)

if(ENABLE_BOBLIGHT_SERVER)
	target_link_libraries(hyperion boblightserver)
endif()

if(ENABLE_EFFECTENGINE)
	target_link_libraries(hyperion effectengine)
endif()

if(ENABLE_FLATBUF_SERVER)
	target_link_libraries(hyperion flatbufserver)
endif()

if(ENABLE_FORWARDER)
	target_link_libraries(hyperion forwarder)
endif()
