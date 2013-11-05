
project(serialport)

include_directories(../../include)

## Sources
set(serial_SRCS
	src/serial.cc
	../../include/serial/serial.h
	../../include/serial/v8stdint.h)

if(UNIX)
	list(APPEND serial_SRCS src/impl/unix.cc)
else()
	list(APPEND serial_SRCS src/impl/win.cc)
endif()

## Add serial library
add_library(${PROJECT_NAME} ${serial_SRCS})

if(UNIX AND NOT APPLE)
	target_link_libraries(${PROJECT_NAME} rt)
endif()
