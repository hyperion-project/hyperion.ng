# Define the current source locations

SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/utils)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/utils)

FILE ( GLOB_RECURSE Utils_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

if ( NOT ENABLE_PROFILER )
	LIST ( REMOVE_ITEM Utils_SOURCES ${CURRENT_HEADER_DIR}/Profiler.h ${CURRENT_SOURCE_DIR}/Profiler.cpp )
endif()

set(Utils_RESOURCES ${CURRENT_SOURCE_DIR}/JSONRPC_schemas.qrc )

qt5_add_resources(Utils_RESOURCES_RCC ${Utils_RESOURCES} OPTIONS "-no-compress")


add_library(hyperion-utils
	${Utils_SOURCES}
	${Utils_RESOURCES}
	${Utils_RESOURCES_RCC}
)

target_link_libraries(hyperion-utils
	hyperion
	Qt5::Core
	Qt5::Gui
	Qt5::Network
)
