project(tinkerforge)

# define the current source/header path
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/dependencies/include/tinkerforge)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/dependencies/build/tinkerforge)

include_directories(${CURRENT_HEADER_DIR})

add_library(tinkerforge
		${CURRENT_HEADER_DIR}/bricklet_led_strip.h
		${CURRENT_HEADER_DIR}/ip_connection.h

		${CURRENT_SOURCE_DIR}/bricklet_led_strip.c
		${CURRENT_SOURCE_DIR}/ip_connection.c)
