cmake_minimum_required(VERSION 2.8)

project(hyperion-v4l2)

# add protocol buffers
find_package(Protobuf REQUIRED)

# find Qt4
find_package(Qt4 REQUIRED QtCore QtGui QtNetwork)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${PROTOBUF_INCLUDE_DIRS}
	${QT_INCLUDES}
)

set(Hyperion_V4L2_HEADERS
	ProtoConnection.h
	ImageHandler.h
	VideoStandardParameter.h
)

set(Hyperion_V4L2_SOURCES
	hyperion-v4l2.cpp
	ProtoConnection.cpp
	ImageHandler.cpp
)

set(Hyperion_V4L2_PROTOS
	${CMAKE_CURRENT_SOURCE_DIR}/../../libsrc/protoserver/message.proto
)

protobuf_generate_cpp(Hyperion_V4L2_PROTO_SRCS Hyperion_V4L2_PROTO_HDRS
	${Hyperion_V4L2_PROTOS}
)

add_executable(hyperion-v4l2
	${Hyperion_V4L2_HEADERS}
	${Hyperion_V4L2_SOURCES}
	${Hyperion_V4L2_PROTO_SRCS}
	${Hyperion_V4L2_PROTO_HDRS}
)

target_link_libraries(hyperion-v4l2
	v4l2-grabber
	getoptPlusPlus
	blackborder
	hyperion-utils
	${PROTOBUF_LIBRARIES}
	pthread
    ${QT_LIBRARIES}
)
