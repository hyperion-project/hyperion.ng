
add_executable(hyperiond
		hyperiond.cpp)

target_link_libraries(hyperiond
		hyperion
		xbmcvideochecker
		effectengine
		jsonserver
		boblightserver
)

if (ENABLE_DISPMANX)
	target_link_libraries(hyperiond dispmanx-grabber)
endif (ENABLE_DISPMANX)

if (ENABLE_FB)
	target_link_libraries(hyperiond framebuffer-grabber)
endif (ENABLE_FB)

if (ENABLE_OSX)
	target_link_libraries(hyperiond osx-grabber)
endif (ENABLE_OSX)

if (ENABLE_V4L2)
	target_link_libraries(hyperiond v4l2-grabber)
endif (ENABLE_V4L2)

if (ENABLE_PROTOBUF)
	target_link_libraries(hyperiond protoserver)
endif (ENABLE_PROTOBUF)
