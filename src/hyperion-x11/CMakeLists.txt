cmake_minimum_required(VERSION 3.0.0)
project(hyperion-x11)

find_package(Qt5Widgets REQUIRED)
find_package(X11 REQUIRED)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/protoserver
	${X11_INCLUDES}
	${PROTOBUF_INCLUDE_DIRS}
)

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
	effectengine
	blackborder
	commandline
	hyperion-utils
	protoserver
	x11-grabber
	${X11_LIBRARIES}
	${X11_Xrender_LIB}
	Qt5::Core
	Qt5::Gui
	Qt5::Network
)

install ( TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin/" COMPONENT "${PLATFORM}" )

if(CMAKE_HOST_UNIX)
	install(CODE "EXECUTE_PROCESS(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\" )" COMPONENT  "${PLATFORM}" )
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "${PLATFORM}" )
	install(CODE "FILE (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME} )" COMPONENT  "${PLATFORM}" )
endif(CMAKE_HOST_UNIX)
