add_subdirectory(hyperiond)

# The following binaries are just compiled ifrequested
if(ENABLE_REMOTE_CTL)
	add_subdirectory(hyperion-remote)
endif()

add_subdirectory(grabberapp)
