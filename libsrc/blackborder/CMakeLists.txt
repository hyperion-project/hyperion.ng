add_library(blackborder
	${CMAKE_SOURCE_DIR}/include/blackborder/BlackBorderDetector.h
	${CMAKE_SOURCE_DIR}/include/blackborder/BlackBorderProcessor.h
	${CMAKE_SOURCE_DIR}/libsrc/blackborder/BlackBorderDetector.cpp
	${CMAKE_SOURCE_DIR}/libsrc/blackborder/BlackBorderProcessor.cpp
)

target_link_libraries(blackborder
 	hyperion-utils
	hyperion
)
