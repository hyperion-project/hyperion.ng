add_library(ssdp
	${CMAKE_SOURCE_DIR}/include/ssdp/SSDPDiscover.h
	${CMAKE_SOURCE_DIR}/include/ssdp/SSDPHandler.h
	${CMAKE_SOURCE_DIR}/include/ssdp/SSDPServer.h
	${CMAKE_SOURCE_DIR}/libsrc/ssdp/SSDPDescription.h
	${CMAKE_SOURCE_DIR}/libsrc/ssdp/SSDPDiscover.cpp
	${CMAKE_SOURCE_DIR}/libsrc/ssdp/SSDPHandler.cpp
	${CMAKE_SOURCE_DIR}/libsrc/ssdp/SSDPServer.cpp
)

target_link_libraries(ssdp
	webserver
)
