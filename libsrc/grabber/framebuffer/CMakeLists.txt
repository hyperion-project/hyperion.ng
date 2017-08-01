
# Find the BCM-package (VC control)
# find_package(BCM REQUIRED)
# include_directories(${BCM_INCLUDE_DIRS})

# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/framebuffer)

SET(FramebufferGrabberHEADERS
	${CURRENT_HEADER_DIR}/FramebufferFrameGrabber.h
	${CURRENT_HEADER_DIR}/FramebufferWrapper.h
)

SET(FramebufferGrabberSOURCES
	${CURRENT_SOURCE_DIR}/FramebufferWrapper.cpp
	${CURRENT_SOURCE_DIR}/FramebufferFrameGrabber.cpp
)

add_library(framebuffer-grabber
	${FramebufferGrabberHEADERS}
	${FramebufferGrabberSOURCES}
)

target_link_libraries(framebuffer-grabber
	hyperion
	${QT_LIBRARIES})
