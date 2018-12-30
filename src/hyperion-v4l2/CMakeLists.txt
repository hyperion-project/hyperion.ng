cmake_minimum_required(VERSION 3.0.0)
project(hyperion-v4l2)

find_package(Qt5Widgets REQUIRED)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/flatbufserver
	${FLATBUFFERS_INCLUDE_DIRS}
)

set(Hyperion_V4L2_HEADERS
	ScreenshotHandler.h
)

set(Hyperion_V4L2_SOURCES
	hyperion-v4l2.cpp
	ScreenshotHandler.cpp
)

add_executable(${PROJECT_NAME}
	${Hyperion_V4L2_HEADERS}
	${Hyperion_V4L2_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
	effectengine
	v4l2-grabber
	commandline
	blackborder
	hyperion-utils
	flatbufserver
	flatbuffers
	ssdp
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
