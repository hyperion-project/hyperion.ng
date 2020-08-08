add_subdirectory(hyperiond)
add_subdirectory(hyperion-remote)

# The following binaries are just compiled if requested
if (ENABLE_AMLOGIC)
	add_subdirectory(hyperion-aml)
endif()

if(ENABLE_V4L2)
	add_subdirectory(hyperion-v4l2)
endif()

if(ENABLE_X11)
	add_subdirectory(hyperion-x11)
endif()

if(ENABLE_XCB)
	add_subdirectory(hyperion-xcb)
endif()

if(ENABLE_DISPMANX)
	add_subdirectory(hyperion-dispmanx)
endif()

if(ENABLE_FB)
	add_subdirectory(hyperion-framebuffer)
endif()

if(ENABLE_QT)
	add_subdirectory(hyperion-qt)
endif()

if(ENABLE_OSX)
	add_subdirectory(hyperion-osx)
endif()
