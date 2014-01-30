# Configure minimum CMAKE version
cmake_minimum_required(VERSION 2.8)

# Set the project name
project(hyperion-x11)

# add protocol buffers
find_package(Protobuf REQUIRED)

# find Qt4
find_package(Qt4 REQUIRED QtCore QtGui QtNetwork)

# Find X11
find_package(X11 REQUIRED)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${PROTOBUF_INCLUDE_DIRS}
	${QT_INCLUDES}
	${X11_INCLUDES}
)

set(Hyperion_X11_QT_HEADERS
	ProtoWrapper.h
	X11Wrapper.h)

set(Hyperion_X11_HEADERS
	X11Grabber.h
	../hyperion-v4l2/ProtoConnection.h
)

set(Hyperion_X11_SOURCES
	hyperion-x11.cpp
	ProtoWrapper.cpp
	X11Grabber.cpp
	X11Wrapper.cpp
	../hyperion-v4l2/ProtoConnection.cpp
)

set(Hyperion_X11_PROTOS
	${CMAKE_CURRENT_SOURCE_DIR}/../../libsrc/protoserver/message.proto
)

QT4_WRAP_CPP(Hyperion_X11_HEADERS_MOC ${Hyperion_X11_QT_HEADERS})

protobuf_generate_cpp(Hyperion_X11_PROTO_SRCS Hyperion_X11_PROTO_HDRS ${Hyperion_X11_PROTOS})

add_executable(hyperion-x11
	${Hyperion_X11_HEADERS}
	${Hyperion_X11_SOURCES}
	${Hyperion_X11_PROTO_SRCS}
	${Hyperion_X11_PROTO_HDRS}
	${Hyperion_X11_HEADERS_MOC}
)

target_link_libraries(hyperion-x11
	getoptPlusPlus
	blackborder
	hyperion-utils
	${PROTOBUF_LIBRARIES}
	${X11_LIBRARIES}
	pthread
)

qt4_use_modules(hyperion-x11
	Core
	Gui
	Network)
