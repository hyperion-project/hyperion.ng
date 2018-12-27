cmake_minimum_required(VERSION 3.0.0)
project(hyperion-dispmanx)

find_package(Qt5Widgets REQUIRED)
IF ( "${PLATFORM}" MATCHES rpi)
	find_package(BCM REQUIRED)
ELSE()
	SET(BCM_INCLUDE_DIRS "")
	SET(BCM_LIBRARIES "")
ENDIF()

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/protoserver
	${BCM_INCLUDE_DIRS}
	${PROTOBUF_INCLUDE_DIRS}
)

set(Hyperion_Dispmanx_HEADERS
	DispmanxWrapper.h
)

set(Hyperion_Dispmanx_SOURCES
	hyperion-dispmanx.cpp
	DispmanxWrapper.cpp
)

add_executable( ${PROJECT_NAME}
	${Hyperion_Dispmanx_HEADERS}
	${Hyperion_Dispmanx_SOURCES}
)

target_link_libraries( ${PROJECT_NAME}
	effectengine
	commandline
	blackborder
	hyperion-utils
	protoclient
	dispmanx-grabber
	${Dispmanx_LIBRARIES}
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
