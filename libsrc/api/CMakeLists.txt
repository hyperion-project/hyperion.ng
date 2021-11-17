# Find the BCM-package (VC control)
if (ENABLE_DISPMANX)
	if ( "${PLATFORM}" MATCHES rpi)
		find_package(BCM REQUIRED)
		include_directories(${BCM_INCLUDE_DIRS})
	else()
		set(BCM_INCLUDE_DIRS "")
		set(BCM_LIBRARIES "")
	endif()
endif (ENABLE_DISPMANX)

# Define the current source locations

SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/api)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/api)

FILE ( GLOB_RECURSE Api_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

set(Api_RESOURCES ${CURRENT_SOURCE_DIR}/JSONRPC_schemas.qrc )

add_library(hyperion-api
	${Api_SOURCES}
	${Api_RESOURCES}
)

if(ENABLE_DX)
	include_directories(${DIRECTX9_INCLUDE_DIRS})
	target_link_libraries(hyperion-api ${DIRECTX9_LIBRARIES})
endif(ENABLE_DX)

target_link_libraries(hyperion-api
	hyperion
	hyperion-utils
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Gui
	Qt${QT_VERSION_MAJOR}::Network
)
