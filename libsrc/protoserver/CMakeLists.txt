
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/protoserver)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/protoserver)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${PROTOBUF_INCLUDE_DIRS}
)

set(ProtoServer_PROTOS ${CURRENT_SOURCE_DIR}/message.proto )

protobuf_generate_cpp(ProtoServer_PROTO_SRCS ProtoServer_PROTO_HDRS ${ProtoServer_PROTOS} )

### Split protoclient from protoserver as protoserver relates to HyperionDaemon and standalone capture binarys can't link to it

add_library(protoclient
	${CURRENT_SOURCE_DIR}/ProtoClientConnection.h
	${CURRENT_SOURCE_DIR}/ProtoClientConnection.cpp
	${ProtoServer_PROTO_SRCS}
	${ProtoServer_PROTO_HDRS}
)

add_library(protoserver
	${CURRENT_HEADER_DIR}/ProtoServer.h
	${CURRENT_SOURCE_DIR}/ProtoServer.cpp
)

# disable warnings for auto generated proto files, we can't change the files ....
SET_SOURCE_FILES_PROPERTIES ( ${ProtoServer_PROTO_SRCS} ${ProtoServer_PROTO_HDRS} ${ProtoServer_PROTOS} PROPERTIES COMPILE_FLAGS -w )

target_link_libraries(protoclient
	hyperion
	hyperion-utils
	protobuf
	Qt5::Gui
)

target_link_libraries(protoserver
	hyperion
	hyperion-utils
	protoclient
	Qt5::Gui
)
