
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/hyperion)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/hyperion)

add_library(hyperion
		${CURRENT_HEADER_DIR}/Hyperion.h
		${CURRENT_HEADER_DIR}/LedDevice.h
		${CURRENT_HEADER_DIR}/LedString.h
		${CURRENT_HEADER_DIR}/ImageToLedsMap.h

		${CURRENT_SOURCE_DIR}/LedDeviceWs2801.h
		${CURRENT_SOURCE_DIR}/LedDeviceWs2801.cpp
		${CURRENT_SOURCE_DIR}/LedString.cpp
		${CURRENT_SOURCE_DIR}/Hyperion.cpp
		${CURRENT_SOURCE_DIR}/ImageToLedsMap.cpp
)

target_link_libraries(hyperion
		hyperion-utils)
