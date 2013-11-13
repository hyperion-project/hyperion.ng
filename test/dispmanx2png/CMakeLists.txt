
# Find the BCM-package (VC control)
find_package(BCM REQUIRED)
include_directories(${BCM_INCLUDE_DIRS})

# Configure the use of QT4
#find_package(Qt4 COMPONENTS QtTest REQUIRED QUIET)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})
link_directories(${CMAKE_FIND_ROOT_PATH}/lib/arm-linux-gnueabihf)


add_executable(dispmanx2png
		dispmanx2png.cpp)

target_link_libraries(dispmanx2png
		dispmanx-grabber
		getoptPlusPlus
		${QT_LIBRARIES})
