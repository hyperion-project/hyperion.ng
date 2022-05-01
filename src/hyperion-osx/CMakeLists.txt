cmake_minimum_required(VERSION 3.1.0)
project(hyperion-osx)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Network Widgets REQUIRED)

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
	flatbufconnect
	flatbuffers
	osx-grabber
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

install ( TARGETS ${PROJECT_NAME} DESTINATION "." COMPONENT "hyperion_osx" )
