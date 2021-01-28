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
endif(ENABLE_OSX)

if (ENABLE_V4L2 OR ENABLE_MF)
	add_subdirectory(video)
endif ()

if (ENABLE_X11)
	add_subdirectory(x11)
endif(ENABLE_X11)

if (ENABLE_XCB)
	add_subdirectory(xcb)
endif(ENABLE_XCB)

if (ENABLE_QT)
	add_subdirectory(qt)
endif(ENABLE_QT)

if (ENABLE_DX)
	add_subdirectory(directx)
endif(ENABLE_DX)

if (ENABLE_AUDIO)
	add_subdirectory(audio)
endif()
