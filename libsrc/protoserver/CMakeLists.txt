
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/protoserver)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/protoserver)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${PROTOBUF_INCLUDE_DIRS}
)

set(ProtoServer_HEADERS
	${CURRENT_HEADER_DIR}/ProtoServer.h
	${CURRENT_HEADER_DIR}/ProtoConnection.h
	${CURRENT_SOURCE_DIR}/ProtoClientConnection.h
	${CURRENT_HEADER_DIR}/ProtoConnectionWrapper.h
)

set(ProtoServer_SOURCES
	${CURRENT_SOURCE_DIR}/ProtoServer.cpp
	${CURRENT_SOURCE_DIR}/ProtoClientConnection.cpp
	${CURRENT_SOURCE_DIR}/ProtoConnection.cpp
	${CURRENT_SOURCE_DIR}/ProtoConnectionWrapper.cpp
)

set(ProtoServer_PROTOS
	${CURRENT_SOURCE_DIR}/message.proto
)

protobuf_generate_cpp(ProtoServer_PROTO_SRCS ProtoServer_PROTO_HDRS
	${ProtoServer_PROTOS}
)

add_library(protoserver
	${ProtoServer_HEADERS}
	${ProtoServer_SOURCES}
	${ProtoServer_PROTOS}
	${ProtoServer_PROTO_SRCS}
	${ProtoServer_PROTO_HDRS}
)

target_link_libraries(protoserver
	hyperion
	hyperion-utils
	protobuf
	Qt5::Gui
)
