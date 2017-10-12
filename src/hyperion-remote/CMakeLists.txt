cmake_minimum_required(VERSION 3.0.0)
project(hyperion-remote)

find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)

# The following I do not undrstand completely...
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

add_executable(${PROJECT_NAME}
	${hyperion-remote_HEADERS}
	${hyperion-remote_SOURCES})

target_link_libraries(${PROJECT_NAME}
	effectengine
	commandline
	hyperion-utils
	Qt5::Gui
	Qt5::Core
	Qt5::Network)

install ( TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin/" COMPONENT "${PLATFORM}" )

if(CMAKE_HOST_UNIX)
	install(CODE "EXECUTE_PROCESS(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\" )" COMPONENT  "${PLATFORM}" )
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "${PLATFORM}" )
	install(CODE "FILE (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME} )" COMPONENT  "${PLATFORM}" )
endif(CMAKE_HOST_UNIX)
