
# Find the BCM-package (VC control)
find_package(BCM REQUIRED)
include_directories(${BCM_INCLUDE_DIRS})

# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/dispmanx)

# Group the headers that go through the MOC compiler
SET(DispmanxGrabberQT_HEADERS
		${CURRENT_HEADER_DIR}/DispmanxWrapper.h
)

SET(DispmanxGrabberHEADERS
		${CURRENT_SOURCE_DIR}/DispmanxFrameGrabber.h
)

SET(DispmanxGrabberSOURCES
		${CURRENT_SOURCE_DIR}/DispmanxWrapper.cpp
		${CURRENT_SOURCE_DIR}/DispmanxFrameGrabber.cpp
)

if(ENABLE_QT5)
QT5_WRAP_CPP(DispmanxGrabberHEADERS_MOC ${DispmanxGrabberQT_HEADERS})
else(ENABLE_QT5)
QT4_WRAP_CPP(DispmanxGrabberHEADERS_MOC ${DispmanxGrabberQT_HEADERS})
endif(ENABLE_QT5)

add_library(dispmanx-grabber
		${DispmanxGrabberHEADERS}
		${DispmanxGrabberQT_HEADERS}
		${DispmanxGrabberHEADERS_MOC}
		${DispmanxGrabberSOURCES}
)

target_link_libraries(dispmanx-grabber
		hyperion
		${QT_LIBRARIES}
		${BCM_LIBRARIES})
