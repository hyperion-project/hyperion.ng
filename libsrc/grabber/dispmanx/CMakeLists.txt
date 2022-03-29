# Find the BCM-package (VC control)
if( "${PLATFORM}" MATCHES rpi)
	find_package(BCM)
	if(BCM_FOUND)
		add_definitions(-DBCM_FOUND)
	endif()
else()
	set(BCM_INCLUDE_DIR "")
	set(BCM_LIBRARY "")
endif()

# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/dispmanx)

FILE ( GLOB DispmanxGrabberSOURCES "${CURRENT_HEADER_DIR}/Dispmanx*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(dispmanx-grabber ${DispmanxGrabberSOURCES})
add_definitions(-DBCM_LIBRARY="${BCM_LIBRARY}")

target_include_directories(dispmanx-grabber PUBLIC
	${BCM_INCLUDE_DIR}
)

target_link_libraries(dispmanx-grabber
	hyperion
	${QT_LIBRARIES}
	${CMAKE_DL_LIBS}
)
