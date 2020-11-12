# Define the current source locations

SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/utils)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/utils)

FILE ( GLOB_RECURSE Utils_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

if ( NOT ENABLE_PROFILER )
	LIST ( REMOVE_ITEM Utils_SOURCES ${CURRENT_HEADER_DIR}/Profiler.h ${CURRENT_SOURCE_DIR}/Profiler.cpp )
endif()

add_library(hyperion-utils
	${Utils_SOURCES}
)

target_link_libraries(hyperion-utils
	hyperion
	Qt6::Core
	Qt6::Gui
	Qt6::Network
)
