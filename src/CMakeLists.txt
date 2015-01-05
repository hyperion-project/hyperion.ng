add_subdirectory(hyperiond)
add_subdirectory(hyperion-remote)

# The following clients depend on the protobuf library
if(ENABLE_PROTOBUF)
    # Add the 'Video 4 Linux' grabber if it is enabled
    if(ENABLE_V4L2)
            add_subdirectory(hyperion-v4l2)
    endif()

    # Add the X11 grabber if it is enabled
    if(ENABLE_X11)
            add_subdirectory(hyperion-x11)
    endif()
endif()
