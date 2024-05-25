add_library(qt-grabber
	${CMAKE_SOURCE_DIR}/include/grabber/qt/QtGrabber.h
	${CMAKE_SOURCE_DIR}/include/grabber/qt/QtWrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/qt/QtGrabber.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/qt/QtWrapper.cpp
)

target_link_libraries(qt-grabber
	hyperion
)
