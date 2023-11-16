find_package(XCB COMPONENTS SHM IMAGE RENDER RANDR REQUIRED)

add_library(xcb-grabber
	${CMAKE_SOURCE_DIR}/include/grabber/xcb/XcbGrabber.h
	${CMAKE_SOURCE_DIR}/include/grabber/xcb/XcbWrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/xcb/XcbCommandExecutor.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/xcb/XcbCommands.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/xcb/XcbGrabber.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/xcb/XcbWrapper.cpp
)

target_link_libraries(xcb-grabber
	hyperion
	${XCB_LIBRARIES}
)

target_include_directories(xcb-grabber PUBLIC
	${XCB_INCLUDE_DIRS}
)
