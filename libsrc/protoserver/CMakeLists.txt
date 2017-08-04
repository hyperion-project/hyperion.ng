
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/protoserver)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/protoserver)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${PROTOBUF_INCLUDE_DIRS}
)
FILE ( GLOB ProtoServer_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

set(ProtoServer_PROTOS ${CURRENT_SOURCE_DIR}/message.proto )

protobuf_generate_cpp(ProtoServer_PROTO_SRCS ProtoServer_PROTO_HDRS ${ProtoServer_PROTOS} )

add_library(protoserver
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
