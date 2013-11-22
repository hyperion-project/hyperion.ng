project(hidapi)

#add libusb and pthreads
find_package(UDev REQUIRED)
find_package(libusb-1.0 REQUIRED)
find_package(Threads REQUIRED)

include_directories(
	../../include/hidapi
	${UDEV_INCLUDE_DIR}
	${LIBUSB_1_INCLUDE_DIRS})

add_library(hidapi-libusb hid-libusb.c)

target_link_libraries(hidapi-libusb
		${LIBUSB_1_LIBRARIES} #apt-get install libusb-1.0-0-dev
		${CMAKE_THREAD_LIBS_INIT}
		${UDEV_LIBRARIES}) # apt-get install libudev-dev
