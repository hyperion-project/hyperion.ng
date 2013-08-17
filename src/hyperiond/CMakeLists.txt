
add_executable(hyperiond
		hyperiond.cpp)

target_link_libraries(hyperiond
		hyperion
		dispmanx-grabber
		jsonserver)
