
add_executable(WriteConfig
		WriteConfig.cpp)

add_executable(boblight-dispmanx
		boblight-dispmanx.cpp
		flagmanager.h
		flagmanager.cpp
		flagmanager-dispmanx.h
		flagmanager-dispmanx.cpp
		grabber-dispmanx.h
		grabber-dispmanx.cpp
		misc.h
		misc.cpp
		timer.h
		timer.cpp
		timeutils.h
		timeutils.cpp)

# Find the BCM-package (VC control)
find_package(BCM REQUIRED)

include_directories(${BCM_INCLUDE_DIRS})

target_link_libraries(boblight-dispmanx
#		hyperion-png
		bob2hyperion
		${BCM_LIBRARIES})

add_executable(HyperionDispmanX
		HyperionDispmanX.cpp)

target_link_libraries(HyperionDispmanX
		hyperion
		${BCM_LIBRARIES})


# Find the libPNG
find_package(PNG QUIET)

if(PNG_FOUND)
	# Add additional includes dirs
	include_directories(${PNG_INCLUDE_DIR})

	add_executable(ViewPng
			ViewPng.cpp)

	target_link_libraries(ViewPng
			hyperion
			hyperion-utils
			${PNG_LIBRARIES})
endif(PNG_FOUND)

add_subdirectory(dispmanx-png)
