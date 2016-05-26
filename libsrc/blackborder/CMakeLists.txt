
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/blackborder)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/blackborder)

SET(Blackborder_HEADERS
		${CURRENT_HEADER_DIR}/BlackBorderDetector.h
		${CURRENT_HEADER_DIR}/BlackBorderProcessor.h
)

SET(Blackborder_SOURCES
		${CURRENT_SOURCE_DIR}/BlackBorderDetector.cpp
		${CURRENT_SOURCE_DIR}/BlackBorderProcessor.cpp
)

add_library(blackborder
		${Blackborder_HEADERS}
		${Blackborder_SOURCES}
)

target_link_libraries(blackborder
		hyperion-utils
)
