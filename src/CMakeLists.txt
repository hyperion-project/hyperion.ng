add_subdirectory(hyperiond)

# The following binaries are just compiled if requested
if (ENABLE_REMOTE_CTL)
	add_subdirectory(hyperion-remote)
endif()

if (ENABLE_AMLOGIC AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-aml)
endif()

if(ENABLE_V4L2 AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-v4l2)
endif()

if(ENABLE_X11 AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-x11)
endif()

if(ENABLE_XCB AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-xcb)
endif()

if(ENABLE_DISPMANX AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-dispmanx)
endif()

if(ENABLE_FB AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-framebuffer)
endif()

if(ENABLE_QT AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-qt)
endif()

if(ENABLE_OSX AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-osx)
endif()
