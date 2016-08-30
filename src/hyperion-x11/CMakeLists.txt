cmake_minimum_required(VERSION 2.8)
project(hyperion-x11)

find_package(Qt5Widgets REQUIRED)
find_package(X11 REQUIRED)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/protoserver
	${QT_INCLUDES}
	${X11_INCLUDES}
	${PROTOBUF_INCLUDE_DIRS}
)

set(Hyperion_X11_QT_HEADERS
	X11Wrapper.h)

set(Hyperion_X11_HEADERS
)

set(Hyperion_X11_SOURCES
	hyperion-x11.cpp
	X11Wrapper.cpp
)

QT5_WRAP_CPP(Hyperion_X11_HEADERS_MOC ${Hyperion_X11_QT_HEADERS})

add_executable(${PROJECT_NAME}
	${Hyperion_X11_HEADERS}
	${Hyperion_X11_SOURCES}
	${Hyperion_X11_HEADERS_MOC}
)

target_link_libraries(${PROJECT_NAME}
	blackborder
	commandline
	hyperion-utils
	protoserver
	x11-grabber
	${X11_LIBRARIES}
	${X11_Xrender_LIB}
	pthread
)

qt5_use_modules(${PROJECT_NAME} Core Gui Network)

install ( TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin/" COMPONENT "${PLATFORM}" )
