cmake_minimum_required(VERSION 3.0.0)
project(hyperion-dx)

find_package(Qt5Widgets REQUIRED)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/flatbufserver
	${FLATBUFFERS_INCLUDE_DIRS}
)

set(Hyperion_DX_HEADERS
	DxWrapper.h
)

set(Hyperion_DX_SOURCES
	DxWrapper.cpp
	hyperion-dx.cpp
)

add_executable(${PROJECT_NAME}
	${Hyperion_DX_HEADERS}
	${Hyperion_DX_SOURCES}
	${WIN_RC_ICON_FILE}
)

target_link_libraries(${PROJECT_NAME}
	commandline
	directx-grabber
	flatbufserver
	flatbuffers
	ssdp
	Qt5::Core
	Qt5::Widgets
	Qt5::Network
)

if(NOT WIN32)
	install ( TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_dx" )
else()
	install ( TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT "hyperion_dx" )
endif()
