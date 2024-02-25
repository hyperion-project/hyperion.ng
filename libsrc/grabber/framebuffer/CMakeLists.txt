add_library(framebuffer-grabber
	${CMAKE_SOURCE_DIR}/include/grabber/framebuffer/FramebufferFrameGrabber.h
	${CMAKE_SOURCE_DIR}/include/grabber/framebuffer/FramebufferWrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/framebuffer/FramebufferFrameGrabber.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/framebuffer/FramebufferWrapper.cpp
)

target_link_libraries(framebuffer-grabber
	hyperion
)
