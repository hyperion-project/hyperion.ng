cmake_minimum_required(VERSION 3.0.0)
project(hyperion-xcb)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/flatbufserver
	${FLATBUFFERS_INCLUDE_DIRS}
)

set(Hyperion_XCB_HEADERS
	XcbWrapper.h
)

set(Hyperion_XCB_SOURCES
	hyperion-xcb.cpp
	XcbWrapper.cpp
)

add_executable(${PROJECT_NAME}
	${Hyperion_XCB_HEADERS}
	${Hyperion_XCB_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
	commandline
	hyperion-utils
	flatbufserver
	flatbuffers
	xcb-grabber
	ssdp
)

install (TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_xcb")

if(CMAKE_HOST_UNIX)
	install(CODE "EXECUTE_PROCESS(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\" )" COMPONENT "hyperion_xcb" )
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_xcb" )
	install(CODE "FILE (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME} )" COMPONENT "hyperion_xcb" )
endif(CMAKE_HOST_UNIX)
