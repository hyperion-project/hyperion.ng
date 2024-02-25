add_library(osx-grabber
	${CMAKE_SOURCE_DIR}/include/grabber/osx/OsxFrameGrabber.h
	${CMAKE_SOURCE_DIR}/include/grabber/osx/OsxWrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/osx/OsxFrameGrabber.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/osx/OsxWrapper.cpp
)

target_link_libraries(osx-grabber
	hyperion
)
