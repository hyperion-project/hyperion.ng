cmake_minimum_required(VERSION 2.8)
project(hyperion-dispmanx)

find_package(Qt5Widgets REQUIRED)
find_package(BCM REQUIRED)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/protoserver
	${QT_INCLUDES}
	${BCM_INCLUDE_DIRS}
	${PROTOBUF_INCLUDE_DIRS}
)

set(Hyperion_Dispmanx_QT_HEADERS
	DispmanxWrapper.h)

set(Hyperion_Dispmanx_HEADERS
)

set(Hyperion_Dispmanx_SOURCES
	hyperion-dispmanx.cpp
	DispmanxWrapper.cpp
)

QT5_WRAP_CPP(Hyperion_Dispmanx_HEADERS_MOC ${Hyperion_Dispmanx_QT_HEADERS})

add_executable( ${PROJECT_NAME}
	${Hyperion_Dispmanx_HEADERS}
	${Hyperion_Dispmanx_SOURCES}
	${Hyperion_Dispmanx_HEADERS_MOC}
)

target_link_libraries( ${PROJECT_NAME}
    commandline
	blackborder
	hyperion-utils
	protoserver
	dispmanx-grabber
	${Dispmanx_LIBRARIES}
	pthread
)

qt5_use_modules(${PROJECT_NAME} Core Gui Network)

install ( TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin/" COMPONENT "${PLATFORM}" )
