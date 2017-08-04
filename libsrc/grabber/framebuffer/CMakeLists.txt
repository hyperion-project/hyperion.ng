# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/framebuffer)

FILE ( GLOB FramebufferGrabberSOURCES "${CURRENT_HEADER_DIR}/Framebuffer*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(framebuffer-grabber ${FramebufferGrabberSOURCES} )

target_link_libraries(framebuffer-grabber
	hyperion
	${QT_LIBRARIES})
