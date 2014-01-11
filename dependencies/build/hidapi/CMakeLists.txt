project(hidapi)

# Add the 'generic' hidapi include directory
include_directories(../../include/hidapi)

#TODO[TvdZ]: Rename hidapi-??? to hidapi
if(APPLE)

	find_package(IOKit          REQUIRED)
	find_package(CoreFoundation REQUIRED)

	include_directories($IOKit_INCLUDE_DIRS})
	include_directories($CoreFoundation_INCLUDE_DIRS})

	# HIDAPI library specific for MacOS
	add_library(hidapi-mac hid-mac.c)

	target_link_libraries(hidapi-mac
			${IOKit_LIBRARY}
			${CoreFoundation_LIBRARY})
elseif(UNIX)
	# HIDAPI library based on lib-usb

	#add libusb and pthreads
	find_package(libusb-1.0 REQUIRED)
	find_package(Threads    REQUIRED)

	include_directories(${LIBUSB_1_INCLUDE_DIRS})

	add_library(hidapi-libusb hid-libusb.c)

	target_link_libraries(hidapi-libusb
			${LIBUSB_1_LIBRARIES} #apt-get install libusb-1.0-0-dev
			${CMAKE_THREAD_LIBS_INIT}
	)

endif()
