cmake_minimum_required(VERSION 3.0.0)
project(hyperion-x11)

find_package(Qt5Widgets REQUIRED)
find_package(X11 REQUIRED)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/flatbufserver
	${X11_INCLUDES}
	${FLATBUFFERS_INCLUDE_DIRS}
)

if(APPLE)
	include_directories("/opt/X11/include")
endif(APPLE)

set(Hyperion_X11_HEADERS
	X11Wrapper.h
)

set(Hyperion_X11_SOURCES
	hyperion-x11.cpp
	X11Wrapper.cpp
)

add_executable(${PROJECT_NAME}
	${Hyperion_X11_HEADERS}
	${Hyperion_X11_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
	commandline
	hyperion-utils
	flatbufserver
	flatbuffers
	x11-grabber
	ssdp
	${X11_LIBRARIES}
	${X11_Xrandr_LIB}
	${X11_Xrender_LIB}
	Qt5::Core
	Qt5::Gui
	Qt5::Network
	Qt5::Widgets
)

install ( TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_x11" )

if(CMAKE_HOST_UNIX)
	install(CODE "EXECUTE_PROCESS(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\" )" COMPONENT "hyperion_x11" )
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_x11" )
	install(CODE "FILE (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME} )" COMPONENT "hyperion_x11" )
endif(CMAKE_HOST_UNIX)
