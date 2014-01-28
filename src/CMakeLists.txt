add_subdirectory(hyperiond)
add_subdirectory(hyperion-remote)

# Add the 'Video 4 Linux' grabber if it is enabled
if(ENABLE_V4L2)
	add_subdirectory(hyperion-v4l2)
endif(ENABLE_V4L2)

# Add the X11 grabber if it is enabled
if(ENABLE_X11)
	add_subdirectory(hyperion-x11)
endif(ENABLE_X11)
