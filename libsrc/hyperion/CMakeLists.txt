
# Find the BCM-package (VC control)
find_package(BCM REQUIRED)
include_directories(${BCM_INCLUDE_DIRS})

# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/hyperion)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/hyperion)

SET(Hyperion_HEADERS
		${CURRENT_HEADER_DIR}/Hyperion.h
		${CURRENT_HEADER_DIR}/LedDevice.h
		${CURRENT_HEADER_DIR}/LedString.h
		${CURRENT_HEADER_DIR}/ImageProcessor.h
		${CURRENT_HEADER_DIR}/ImageProcessorFactory.h
		${CURRENT_HEADER_DIR}/PriorityMuxer.h
		${CURRENT_HEADER_DIR}/DispmanxWrapper.h

		${CURRENT_SOURCE_DIR}/DispmanxFrameGrabber.h

		${CURRENT_SOURCE_DIR}/LedDeviceWs2801.h
		${CURRENT_SOURCE_DIR}/ImageToLedsMap.h
		${CURRENT_SOURCE_DIR}/ColorTransform.h
)

SET(Hyperion_SOURCES
		${CURRENT_SOURCE_DIR}/LedString.cpp
		${CURRENT_SOURCE_DIR}/Hyperion.cpp
		${CURRENT_SOURCE_DIR}/ImageProcessor.cpp
		${CURRENT_SOURCE_DIR}/ImageProcessorFactory.cpp
		${CURRENT_SOURCE_DIR}/PriorityMuxer.cpp

		${CURRENT_SOURCE_DIR}/DispmanxWrapper.cpp
		${CURRENT_SOURCE_DIR}/DispmanxFrameGrabber.cpp

		${CURRENT_SOURCE_DIR}/LedDeviceWs2801.cpp
		${CURRENT_SOURCE_DIR}/ImageToLedsMap.cpp
		${CURRENT_SOURCE_DIR}/ColorTransform.cpp
)

QT4_WRAP_CPP(Hyperion_HEADERS_MOC ${Hyperion_HEADERS})

add_library(hyperion
		${Hyperion_HEADERS}
		${Hyperion_HEADERS_MOC}
		${Hyperion_SOURCES}
)

target_link_libraries(hyperion
		hyperion-utils
		${QT_LIBRARIES}
		${BCM_LIBRARIES})
