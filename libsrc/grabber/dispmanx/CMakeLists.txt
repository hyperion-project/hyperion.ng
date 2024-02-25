# Find the BCM-package (VC control)
if("${PLATFORM}" MATCHES rpi)
	find_package(BCM)
	if(BCM_FOUND)
		add_definitions(-DBCM_FOUND)
	endif()
else()
	set(BCM_INCLUDE_DIR "")
	set(BCM_LIBRARY "")
endif()

add_library(dispmanx-grabber
	${CMAKE_SOURCE_DIR}/include/grabber/dispmanx/DispmanxFrameGrabber.h
	${CMAKE_SOURCE_DIR}/include/grabber/dispmanx/DispmanxWrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/dispmanx/DispmanxFrameGrabber.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/dispmanx/DispmanxWrapper.cpp
)

add_definitions(-DBCM_LIBRARY="${BCM_LIBRARY}")

target_link_libraries(dispmanx-grabber
	hyperion
	${CMAKE_DL_LIBS}
)

target_include_directories(dispmanx-grabber PUBLIC
	${BCM_INCLUDE_DIR}
)
