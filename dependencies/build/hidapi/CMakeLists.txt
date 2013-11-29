project(hidapi)

#add libusb and pthreads
find_package(libusb-1.0 REQUIRED)
find_package(Threads REQUIRED)

include_directories(
	../../include/hidapi
	${LIBUSB_1_INCLUDE_DIRS})

add_library(hidapi-libusb hid-libusb.c)

target_link_libraries(hidapi-libusb
		${LIBUSB_1_LIBRARIES} #apt-get install libusb-1.0-0-dev
		${CMAKE_THREAD_LIBS_INIT}
)
