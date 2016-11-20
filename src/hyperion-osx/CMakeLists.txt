cmake_minimum_required(VERSION 2.8)
project(hyperion-osx)

find_package(Qt5Widgets REQUIRED)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/protoserver
	${QT_INCLUDES}
	${PROTOBUF_INCLUDE_DIRS}
)

set(Hyperion_OSX_QT_HEADERS
	OsxWrapper.h
)

set(Hyperion_OSX_HEADERS
)

set(Hyperion_OSX_SOURCES
	hyperion-osx.cpp
	OsxWrapper.cpp
)

QT5_WRAP_CPP(Hyperion_OSX_HEADERS_MOC ${Hyperion_OSX_QT_HEADERS})

add_executable( ${PROJECT_NAME}
	${Hyperion_OSX_HEADERS}
	${Hyperion_OSX_SOURCES}
	${Hyperion_OSX_HEADERS_MOC}
)

target_link_libraries( ${PROJECT_NAME}
	effectengine
	commandline
	blackborder
	hyperion-utils
	protoserver
	osx-grabber
	pthread
)

qt5_use_modules(${PROJECT_NAME} Core Gui Network)

install ( TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin/" COMPONENT "${PLATFORM}" )

if(CMAKE_HOST_UNIX)
	install(CODE "EXECUTE_PROCESS(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\" )" COMPONENT  "${PLATFORM}" )
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "${PLATFORM}" )
	install(CODE "FILE (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME} )" COMPONENT  "${PLATFORM}" )
endif(CMAKE_HOST_UNIX)
