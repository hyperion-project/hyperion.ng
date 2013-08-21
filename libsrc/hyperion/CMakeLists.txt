
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/hyperion)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/hyperion)

# Group the headers that go through the MOC compiler
SET(Hyperion_QT_HEADERS
		${CURRENT_HEADER_DIR}/Hyperion.h
)

SET(Hyperion_HEADERS
		${CURRENT_HEADER_DIR}/LedString.h
		${CURRENT_HEADER_DIR}/LedDevice.h
		${CURRENT_HEADER_DIR}/ImageProcessor.h
		${CURRENT_HEADER_DIR}/ImageProcessorFactory.h
		${CURRENT_HEADER_DIR}/PriorityMuxer.h

		${CURRENT_SOURCE_DIR}/LedDeviceWs2801.h
		${CURRENT_SOURCE_DIR}/LedDeviceTest.h
		${CURRENT_SOURCE_DIR}/ImageToLedsMap.h
		${CURRENT_SOURCE_DIR}/BlackBorderDetector.h
		${CURRENT_SOURCE_DIR}/ColorTransform.h
)

SET(Hyperion_SOURCES
		${CURRENT_SOURCE_DIR}/LedString.cpp
		${CURRENT_SOURCE_DIR}/Hyperion.cpp
		${CURRENT_SOURCE_DIR}/ImageProcessor.cpp
		${CURRENT_SOURCE_DIR}/ImageProcessorFactory.cpp
		${CURRENT_SOURCE_DIR}/PriorityMuxer.cpp

		${CURRENT_SOURCE_DIR}/LedDeviceWs2801.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceTest.cpp
		${CURRENT_SOURCE_DIR}/ImageToLedsMap.cpp
		${CURRENT_SOURCE_DIR}/BlackBorderDetector.cpp
		${CURRENT_SOURCE_DIR}/ColorTransform.cpp
)

set(Hyperion_RESOURCES
		${CURRENT_SOURCE_DIR}/resource.qrc
)

QT4_WRAP_CPP(Hyperion_HEADERS_MOC ${Hyperion_QT_HEADERS})

qt4_add_resources(Hyperion_RESOURCES_RCC ${Hyperion_RESOURCES} OPTIONS "-no-compress")

add_library(hyperion
		${Hyperion_HEADERS}
		${Hyperion_QT_HEADERS}
		${Hyperion_HEADERS_MOC}
		${Hyperion_SOURCES}
		${Hyperion_RESOURCES_RCC}
)

target_link_libraries(hyperion
		hyperion-utils
		${QT_LIBRARIES})
