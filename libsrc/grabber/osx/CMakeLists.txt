add_library(osx-grabber
	${CMAKE_SOURCE_DIR}/include/grabber/osx/OsxFrameGrabber.h
	${CMAKE_SOURCE_DIR}/include/grabber/osx/OsxWrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/osx/OsxFrameGrabber.mm
	${CMAKE_SOURCE_DIR}/libsrc/grabber/osx/OsxWrapper.cpp
)

target_link_libraries(osx-grabber
	hyperion
	"-framework CoreGraphics"
	"-framework Foundation"
)

find_library(ScreenCaptureKit ScreenCaptureKit)
if(ScreenCaptureKit)
	target_compile_definitions(osx-grabber PRIVATE SDK_15_AVAILABLE)
	target_link_libraries(osx-grabber "${ScreenCaptureKit}")
endif()
