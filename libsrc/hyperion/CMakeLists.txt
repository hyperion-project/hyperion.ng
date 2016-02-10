
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/hyperion)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/hyperion)

# Group the headers that go through the MOC compiler
SET(Hyperion_QT_HEADERS
		${CURRENT_HEADER_DIR}/Hyperion.h

		${CURRENT_SOURCE_DIR}/LinearColorSmoothing.h
)

SET(Hyperion_HEADERS
		${CURRENT_HEADER_DIR}/ImageProcessor.h
		${CURRENT_HEADER_DIR}/ImageProcessorFactory.h
		${CURRENT_HEADER_DIR}/ImageToLedsMap.h
		${CURRENT_HEADER_DIR}/LedString.h
		${CURRENT_HEADER_DIR}/PriorityMuxer.h

		${CURRENT_SOURCE_DIR}/MultiColorTransform.h
)

SET(Hyperion_SOURCES
		${CURRENT_SOURCE_DIR}/Hyperion.cpp
		${CURRENT_SOURCE_DIR}/ImageProcessor.cpp
		${CURRENT_SOURCE_DIR}/ImageProcessorFactory.cpp
		${CURRENT_SOURCE_DIR}/LedString.cpp
		${CURRENT_SOURCE_DIR}/PriorityMuxer.cpp

		${CURRENT_SOURCE_DIR}/ImageToLedsMap.cpp
		${CURRENT_SOURCE_DIR}/MultiColorTransform.cpp
		${CURRENT_SOURCE_DIR}/LinearColorSmoothing.cpp
)

set(Hyperion_RESOURCES
		${CURRENT_SOURCE_DIR}/resource.qrc
)

if(ENABLE_QT5)
QT5_WRAP_CPP(Hyperion_HEADERS_MOC ${Hyperion_QT_HEADERS})
QT5_ADD_RESOURCES(Hyperion_RESOURCES_RCC ${Hyperion_RESOURCES} OPTIONS "-no-compress")
else(ENABLE_QT5)
QT4_WRAP_CPP(Hyperion_HEADERS_MOC ${Hyperion_QT_HEADERS})
QT4_ADD_RESOURCES(Hyperion_RESOURCES_RCC ${Hyperion_RESOURCES} OPTIONS "-no-compress")
endif(ENABLE_QT5)

add_library(hyperion
		${Hyperion_HEADERS}
		${Hyperion_QT_HEADERS}
		${Hyperion_HEADERS_MOC}
		${Hyperion_SOURCES}
		${Hyperion_RESOURCES_RCC}
)

if(ENABLE_QT5)
qt5_use_modules(hyperion Widgets)
endif(ENABLE_QT5)

target_link_libraries(hyperion
		blackborder
		hyperion-utils
		leddevice
		effectengine
		serialport
		${QT_LIBRARIES}
)
