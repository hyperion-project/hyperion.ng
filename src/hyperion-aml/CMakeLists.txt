cmake_minimum_required(VERSION 3.0.0)
project(hyperion-aml)

find_package(Qt5Widgets REQUIRED)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/protoserver
	${PROTOBUF_INCLUDE_DIRS}
)

set(Hyperion_AML_HEADERS
	AmlogicWrapper.h
)

set(Hyperion_AML_SOURCES
	hyperion-aml.cpp
	AmlogicWrapper.cpp
)

add_executable(${PROJECT_NAME}
	${Hyperion_AML_HEADERS}
	${Hyperion_AML_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
	effectengine
	commandline
	blackborder
	hyperion-utils
	protoserver
	amlogic-grabber
	framebuffer-grabber
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
