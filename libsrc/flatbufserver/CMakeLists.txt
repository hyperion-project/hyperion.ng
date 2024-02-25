# set (compiled) Flatbuffer schema names
set(FBS_Request "hyperion_request")
set(FBS_Reply "hyperion_reply")

# define and compile flatbuffer schemas
list(APPEND Compiled_FBS ${FBS_Request}_generated.h)
list(APPEND Compiled_FBS ${FBS_Reply}_generated.h)
compile_flattbuffer_schema(${CMAKE_CURRENT_SOURCE_DIR}/${FBS_Request}.fbs ${CMAKE_CURRENT_SOURCE_DIR})
compile_flattbuffer_schema(${CMAKE_CURRENT_SOURCE_DIR}/${FBS_Reply}.fbs ${CMAKE_CURRENT_SOURCE_DIR})

# let cmake know about new generated source files
set_source_files_properties(${Compiled_FBS} PROPERTIES GENERATED TRUE)

# split flatbufconnect from flatbufserver as flatbufserver relates to HyperionDaemon
if(ENABLE_FLATBUF_CONNECT)
	add_library(flatbufconnect
		${CMAKE_SOURCE_DIR}/include/flatbufserver/FlatBufferConnection.h
		${CMAKE_SOURCE_DIR}/libsrc/flatbufserver/FlatBufferConnection.cpp
		${Compiled_FBS}
	)

	target_link_libraries(flatbufconnect
		hyperion-utils
		flatbuffers
	)

	target_include_directories(flatbufconnect PUBLIC
		${FLATBUFFERS_INCLUDE_DIRS}
	)
endif()

if(ENABLE_FLATBUF_SERVER)
	add_library(flatbufserver
		${CMAKE_SOURCE_DIR}/include/flatbufserver/FlatBufferServer.h
		${CMAKE_SOURCE_DIR}/libsrc/flatbufserver/FlatBufferServer.cpp
		${CMAKE_SOURCE_DIR}/libsrc/flatbufserver/FlatBufferClient.h
		${CMAKE_SOURCE_DIR}/libsrc/flatbufserver/FlatBufferClient.cpp
		${Compiled_FBS}
	)

	target_link_libraries(flatbufserver
		hyperion-utils
		flatbuffers
	)

	target_include_directories(flatbufserver PUBLIC
		${FLATBUFFERS_INCLUDE_DIRS}
	)

	if(ENABLE_MDNS)
		target_link_libraries(flatbufserver mdns)
	endif()
endif()

