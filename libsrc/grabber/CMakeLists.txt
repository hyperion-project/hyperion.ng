if (ENABLE_AMLOGIC)
	add_subdirectory(amlogic)
endif (ENABLE_AMLOGIC)

if (ENABLE_DISPMANX)
	add_subdirectory(dispmanx)
endif (ENABLE_DISPMANX)

if (ENABLE_FB)
	add_subdirectory(framebuffer)
endif (ENABLE_FB)

if (ENABLE_OSX)
	add_subdirectory(osx)
endif()

if (ENABLE_V4L2)
	add_subdirectory(v4l2)
endif (ENABLE_V4L2)

if (ENABLE_X11)
	add_subdirectory(x11)
endif()

if (ENABLE_XCB)
	add_subdirectory(xcb)
endif()

if (ENABLE_QT)
	add_subdirectory(qt)
endif()
