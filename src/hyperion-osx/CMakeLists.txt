cmake_minimum_required(VERSION 3.0.0)
project(hyperion-osx)

find_package(Qt5Widgets REQUIRED)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/flatbufserver
	${FLATBUFFERS_INCLUDE_DIRS}
)

set(Hyperion_OSX_HEADERS
	OsxWrapper.h
)

set(Hyperion_OSX_SOURCES
	hyperion-osx.cpp
	OsxWrapper.cpp
)

add_executable( ${PROJECT_NAME}
	${Hyperion_OSX_HEADERS}
	${Hyperion_OSX_SOURCES}
)

target_link_libraries( ${PROJECT_NAME}
	commandline
	hyperion-utils
	flatbufserver
	flatbuffers
	osx-grabber
	ssdp
	Qt5::Core
	Qt5::Gui
	Qt5::Network
)

install ( TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_osx" )

if(CMAKE_HOST_UNIX)
	install(CODE "EXECUTE_PROCESS(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\" )" COMPONENT "hyperion_osx" )
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_osx" )
	install(CODE "FILE (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME} )" COMPONENT "hyperion_osx" )
endif(CMAKE_HOST_UNIX)
