add_library(boblightserver
	${CMAKE_SOURCE_DIR}/include/boblightserver/BoblightServer.h
	${CMAKE_SOURCE_DIR}/libsrc/boblightserver/BoblightServer.cpp
	${CMAKE_SOURCE_DIR}/libsrc/boblightserver/BoblightClientConnection.h
	${CMAKE_SOURCE_DIR}/libsrc/boblightserver/BoblightClientConnection.cpp
)

target_link_libraries(boblightserver
	hyperion
	hyperion-utils
)
