# Find the BCM-package (VC control)
IF ( "${PLATFORM}" MATCHES rpi)
	find_package(BCM REQUIRED)
	include_directories(${BCM_INCLUDE_DIRS})
ELSE()
	SET(BCM_INCLUDE_DIRS "")
	SET(BCM_LIBRARIES "")
ENDIF()

# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/dispmanx)

FILE ( GLOB DispmanxGrabberSOURCES "${CURRENT_HEADER_DIR}/Dispmanx*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(dispmanx-grabber ${DispmanxGrabberSOURCES} )

target_link_libraries(dispmanx-grabber
	hyperion
	${QT_LIBRARIES}
	${BCM_LIBRARIES}
)
