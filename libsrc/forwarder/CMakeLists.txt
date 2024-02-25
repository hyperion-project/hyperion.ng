add_library(forwarder
	${CMAKE_SOURCE_DIR}/include/forwarder/MessageForwarder.h
	${CMAKE_SOURCE_DIR}/libsrc/forwarder/MessageForwarder.cpp
)

target_link_libraries(forwarder
	hyperion
	hyperion-utils
)

if(ENABLE_FLATBUF_CONNECT)
	target_link_libraries(forwarder flatbufconnect)
endif()
