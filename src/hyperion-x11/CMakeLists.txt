cmake_minimum_required(VERSION 3.1.0)
project(hyperion-x11)

find_package(X11 REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Network Widgets REQUIRED)


include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/flatbufserver
	${X11_INCLUDES}
	${FLATBUFFERS_INCLUDE_DIRS}
)

if(APPLE)
	include_directories("/opt/X11/include")
endif(APPLE)

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
	commandline
	hyperion-utils
	flatbufconnect
	flatbuffers
	x11-grabber
	${X11_LIBRARIES}
	${X11_Xrandr_LIB}
	${X11_Xrender_LIB}
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Gui
	Qt${QT_VERSION_MAJOR}::Network
	Qt${QT_VERSION_MAJOR}::Widgets
)

if(ENABLE_MDNS)
        target_link_libraries(${PROJECT_NAME} mdns)
else()
        target_link_libraries(${PROJECT_NAME} ssdp)
endif()

install ( TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_x11" )

if(CMAKE_HOST_UNIX)
	install(CODE "EXECUTE_PROCESS(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\" )" COMPONENT "hyperion_x11" )
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_x11" )
	install(CODE "FILE (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME} )" COMPONENT "hyperion_x11" )
endif(CMAKE_HOST_UNIX)
