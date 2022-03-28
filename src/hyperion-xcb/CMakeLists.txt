cmake_minimum_required(VERSION 3.1.0)
project(hyperion-xcb)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network Widgets REQUIRED)

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
	flatbufconnect
	flatbuffers
	xcb-grabber
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Network
	Qt${QT_VERSION_MAJOR}::Widgets
)

if(ENABLE_MDNS)
        target_link_libraries(${PROJECT_NAME} mdns)
else()
        target_link_libraries(${PROJECT_NAME} ssdp)
endif()

install (TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_xcb")

if(CMAKE_HOST_UNIX)
	install(CODE "EXECUTE_PROCESS(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\" )" COMPONENT "hyperion_xcb" )
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_xcb" )
	install(CODE "FILE (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME} )" COMPONENT "hyperion_xcb" )
endif(CMAKE_HOST_UNIX)
