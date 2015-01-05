cmake_minimum_required(VERSION 2.8)

project(hyperion-v4l2)

# find Qt4
find_package(Qt4 REQUIRED QtCore QtGui QtNetwork)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/protoserver
	${PROTOBUF_INCLUDE_DIRS}
	${QT_INCLUDES}
)

set(Hyperion_V4L2_QT_HEADERS
	ScreenshotHandler.h
)

set(Hyperion_V4L2_HEADERS
	VideoStandardParameter.h
	PixelFormatParameter.h
)

set(Hyperion_V4L2_SOURCES
	hyperion-v4l2.cpp
	ScreenshotHandler.cpp
)

QT4_WRAP_CPP(Hyperion_V4L2_MOC_SOURCES ${Hyperion_V4L2_QT_HEADERS})

add_executable(hyperion-v4l2
	${Hyperion_V4L2_HEADERS}
	${Hyperion_V4L2_SOURCES}
	${Hyperion_V4L2_QT_HEADERS}
	${Hyperion_V4L2_MOC_SOURCES}
)

target_link_libraries(hyperion-v4l2
	v4l2-grabber
	getoptPlusPlus
	blackborder
	hyperion-utils
	protoserver
	pthread
	${QT_LIBRARIES}
)
