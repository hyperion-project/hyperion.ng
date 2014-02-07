
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/protoserver)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/protoserver)

include_directories(
		${CMAKE_CURRENT_BINARY_DIR}
		${PROTOBUF_INCLUDE_DIRS})

# Group the headers that go through the MOC compiler
set(ProtoServer_QT_HEADERS
		${CURRENT_HEADER_DIR}/ProtoServer.h
		${CURRENT_SOURCE_DIR}/ProtoClientConnection.h
)

set(ProtoServer_HEADERS
)

set(ProtoServer_SOURCES
		${CURRENT_SOURCE_DIR}/ProtoServer.cpp
		${CURRENT_SOURCE_DIR}/ProtoClientConnection.cpp
)

set(ProtoServer_PROTOS
		${CURRENT_SOURCE_DIR}/message.proto
)

protobuf_generate_cpp(ProtoServer_PROTO_SRCS ProtoServer_PROTO_HDRS
		${ProtoServer_PROTOS}
)

qt4_wrap_cpp(ProtoServer_HEADERS_MOC ${ProtoServer_QT_HEADERS})

add_library(protoserver
		${ProtoServer_HEADERS}
		${ProtoServer_QT_HEADERS}
		${ProtoServer_SOURCES}
		${ProtoServer_HEADERS_MOC}
		${ProtoServer_PROTOS}
		${ProtoServer_PROTO_SRCS}
		${ProtoServer_PROTO_HDRS}
)

target_link_libraries(protoserver
		hyperion
		hyperion-utils
        ${PROTOBUF_LIBRARIES}
        ${QT_LIBRARIES})
