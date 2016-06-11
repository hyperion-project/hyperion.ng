
# Find the BCM-package (VC control)
# find_package(BCM REQUIRED)
# include_directories(${BCM_INCLUDE_DIRS})

# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/framebuffer)

# Group the headers that go through the MOC compiler
SET(FramebufferGrabberQT_HEADERS
	${CURRENT_HEADER_DIR}/FramebufferWrapper.h
)

SET(FramebufferGrabberHEADERS
	${CURRENT_HEADER_DIR}/FramebufferFrameGrabber.h
)

SET(FramebufferGrabberSOURCES
	${CURRENT_SOURCE_DIR}/FramebufferWrapper.cpp
	${CURRENT_SOURCE_DIR}/FramebufferFrameGrabber.cpp
)

if(ENABLE_QT5)
	QT5_WRAP_CPP(FramebufferGrabberHEADERS_MOC ${FramebufferGrabberQT_HEADERS})
else()
	QT4_WRAP_CPP(FramebufferGrabberHEADERS_MOC ${FramebufferGrabberQT_HEADERS})
endif()

add_library(framebuffer-grabber
	${FramebufferGrabberHEADERS}
	${FramebufferGrabberQT_HEADERS}
	${FramebufferGrabberHEADERS_MOC}
	${FramebufferGrabberSOURCES}
)

target_link_libraries(framebuffer-grabber
	hyperion
	${QT_LIBRARIES})
