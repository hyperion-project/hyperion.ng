# this is only available on real pi
IF ( "${PLATFORM}" MATCHES rpi)

	find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui REQUIRED)

	# Find the BCM-package (VC control)
	find_package(BCM REQUIRED)
	include_directories(${BCM_INCLUDE_DIRS})

	add_definitions(${QT_DEFINITIONS})
	link_directories(${CMAKE_FIND_ROOT_PATH}/lib/arm-linux-gnueabihf)

	add_executable(dispmanx2png
		dispmanx2png.cpp)

	target_link_libraries(dispmanx2png
		dispmanx-grabber
		Qt${QT_VERSION_MAJOR::Gui)
ENDIF()
