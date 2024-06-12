add_compile_definitions("_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR")

# set and compile proto schema
set(ProtoServer_PROTOS ${CMAKE_SOURCE_DIR}/libsrc/protoserver/message.proto)
protobuf_generate_cpp(ProtoServer_PROTO_SRCS ProtoServer_PROTO_HDRS ${ProtoServer_PROTOS})

# disable warnings for auto generated proto files, we can't change the files ....
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	set_source_files_properties(${ProtoServer_PROTO_SRCS} ${ProtoServer_PROTO_HDRS} ${ProtoServer_PROTOS} PROPERTIES COMPILE_FLAGS "-w -Wno-return-local-addr")
elseif(MSVC)
	set_source_files_properties(${ProtoServer_PROTO_SRCS} ${ProtoServer_PROTO_HDRS} ${ProtoServer_PROTOS} PROPERTIES COMPILE_FLAGS "/W0")
endif()

### Split protoclient from protoserver as protoserver relates to HyperionDaemon and standalone capture binarys can't link to it
add_library(protoclient
	${CMAKE_SOURCE_DIR}/libsrc/protoserver/ProtoClientConnection.h
	${CMAKE_SOURCE_DIR}/libsrc/protoserver/ProtoClientConnection.cpp
	${ProtoServer_PROTO_SRCS}
	${ProtoServer_PROTO_HDRS}
)

target_link_libraries(protoclient
	hyperion
	hyperion-utils
	${PROTOBUF_LIBRARIES}
	Qt${QT_VERSION_MAJOR}::Gui
)

target_include_directories(protoclient PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${PROTOBUF_INCLUDE_DIRS}
)

add_library(protoserver
	${CMAKE_SOURCE_DIR}/include/protoserver/ProtoServer.h
	${CMAKE_SOURCE_DIR}/libsrc/protoserver/ProtoServer.cpp
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
