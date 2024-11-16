add_library(osx-grabber
	${CMAKE_SOURCE_DIR}/include/grabber/osx/OsxFrameGrabber.h
	${CMAKE_SOURCE_DIR}/include/grabber/osx/OsxWrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/osx/OsxFrameGrabber.mm
	${CMAKE_SOURCE_DIR}/libsrc/grabber/osx/OsxWrapper.cpp
)

target_link_libraries(osx-grabber
	hyperion
	"$<LINK_LIBRARY:FRAMEWORK,CoreGraphics.framework>"
)

file(WRITE ${CMAKE_BINARY_DIR}/tmp/SDK15Available.c
	"#include <AvailabilityMacros.h>
	#if __MAC_OS_X_VERSION_MAX_ALLOWED < 150000
	#error __MAC_OS_X_VERSION_MAX_ALLOWED < 150000
	#endif
	int main(int argc, char** argv)
	{
		return 0;
	}"
)

try_compile(SDK_15_AVAILABLE ${CMAKE_BINARY_DIR} SOURCES ${CMAKE_BINARY_DIR}/tmp/SDK15Available.c)
if(SDK_15_AVAILABLE)
	target_compile_definitions(osx-grabber PRIVATE SDK_15_AVAILABLE)
	target_link_libraries(osx-grabber "$<LINK_LIBRARY:WEAK_FRAMEWORK,ScreenCaptureKit.framework>")
endif()
