
add_executable(hyperiond
		hyperiond.cpp)

target_link_libraries(hyperiond
		bootsequence
		hyperion
		dispmanx-grabber
		xbmcvideochecker
		jsonserver)
