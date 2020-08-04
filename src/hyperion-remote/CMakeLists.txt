cmake_minimum_required(VERSION 3.0.0)
project(hyperion-remote)

find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)

# The following I do not understand completely...
# libQtCore.so uses some hardcoded library path inside which are incorrect after copying the file RPi file system
# Therefore, an extra path is needed on which to find the required libraries
IF ( EXISTS ${CMAKE_FIND_ROOT_PATH}/lib/arm-linux-gnueabihf )
	LINK_DIRECTORIES(${LINK_DIRECTORIES} ${CMAKE_FIND_ROOT_PATH}/lib/arm-linux-gnueabihf)
ENDIF()

set(hyperion-remote_HEADERS
	JsonConnection.h)

set(hyperion-remote_SOURCES
	hyperion-remote.cpp
	JsonConnection.cpp)

# generate windows .rc file for this binary
if (WIN32)
	include(${CMAKE_SOURCE_DIR}/cmake/win/win_rc.cmake)
	generate_win_rc_file(${PROJECT_NAME})
endif()

add_executable(${PROJECT_NAME}
	${hyperion-remote_HEADERS}
	${hyperion-remote_SOURCES}
	${${PROJECT_NAME}_WIN_RC_PATH}
)

target_link_libraries(${PROJECT_NAME}
	effectengine
	commandline
	hyperion-utils
	ssdp
	Qt5::Gui
	Qt5::Core
	Qt5::Network)

if (ENABLE_AMLOGIC)
	target_link_libraries(${PROJECT_NAME}
		pcre16 dl z
	)
endif()

if(NOT WIN32)
	install ( TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_remote" )
else()
	install ( TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT "hyperion_remote" )
endif()

if(CMAKE_HOST_UNIX)
	install(CODE "EXECUTE_PROCESS(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\" )" COMPONENT "hyperion_remote" )
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_remote" )
	install(CODE "FILE (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME} )" COMPONENT "hyperion_remote" )
endif(CMAKE_HOST_UNIX)
