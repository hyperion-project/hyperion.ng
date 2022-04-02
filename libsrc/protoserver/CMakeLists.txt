
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
if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	set_source_files_properties(${ProtoServer_PROTO_SRCS} ${ProtoServer_PROTO_HDRS} ${ProtoServer_PROTOS} PROPERTIES COMPILE_FLAGS "-w -Wno-return-local-addr")
elseif(MSVC)
	set_source_files_properties(${ProtoServer_PROTO_SRCS} ${ProtoServer_PROTO_HDRS} ${ProtoServer_PROTOS} PROPERTIES COMPILE_FLAGS "/W0")
endif()

target_link_libraries(protoclient
	hyperion
	hyperion-utils
	${PROTOBUF_LIBRARIES}
	Qt${QT_VERSION_MAJOR}::Gui
)

target_link_libraries(protoserver
	hyperion
	hyperion-utils
	protoclient
	Qt${QT_VERSION_MAJOR}::Gui
)

if(ENABLE_MDNS)
	target_link_libraries(protoserver mdns)
endif()
