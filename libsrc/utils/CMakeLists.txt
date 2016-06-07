# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/utils)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/utils)

if ( ENABLE_PROFILER )
	SET ( PROFILER_SOURCE ${CURRENT_HEADER_DIR}/Profiler.h ${CURRENT_SOURCE_DIR}/Profiler.cpp )
endif()

add_library(hyperion-utils
	${CURRENT_HEADER_DIR}/ColorArgb.h
	${CURRENT_SOURCE_DIR}/ColorArgb.cpp
	${CURRENT_HEADER_DIR}/ColorBgr.h
	${CURRENT_SOURCE_DIR}/ColorBgr.cpp
	${CURRENT_HEADER_DIR}/ColorRgb.h
	${CURRENT_SOURCE_DIR}/ColorRgb.cpp
	${CURRENT_HEADER_DIR}/ColorRgba.h
	${CURRENT_SOURCE_DIR}/ColorRgba.cpp
	${CURRENT_HEADER_DIR}/ColorRgbw.h
	${CURRENT_SOURCE_DIR}/ColorRgbw.cpp
	${CURRENT_HEADER_DIR}/Image.h
	${CURRENT_HEADER_DIR}/Sleep.h

	${CURRENT_HEADER_DIR}/Logger.h
	${CURRENT_SOURCE_DIR}/Logger.cpp
	${PROFILER_SOURCE}

	${CURRENT_HEADER_DIR}/PixelFormat.h
	${CURRENT_HEADER_DIR}/VideoMode.h

	${CURRENT_HEADER_DIR}/ImageResampler.h
	${CURRENT_SOURCE_DIR}/ImageResampler.cpp

	${CURRENT_HEADER_DIR}/HsvTransform.h
	${CURRENT_SOURCE_DIR}/HsvTransform.cpp
	${CURRENT_HEADER_DIR}/HslTransform.h
	${CURRENT_SOURCE_DIR}/HslTransform.cpp
	${CURRENT_HEADER_DIR}/RgbChannelTransform.h
	${CURRENT_SOURCE_DIR}/RgbChannelTransform.cpp
	${CURRENT_HEADER_DIR}/RgbChannelCorrection.h
	${CURRENT_SOURCE_DIR}/RgbChannelCorrection.cpp
	${CURRENT_HEADER_DIR}/RgbChannelAdjustment.h
	${CURRENT_SOURCE_DIR}/RgbChannelAdjustment.cpp

	${CURRENT_HEADER_DIR}/RgbToRgbw.h
	${CURRENT_SOURCE_DIR}/RgbToRgbw.cpp

	${CURRENT_HEADER_DIR}/jsonschema/JsonFactory.h
	${CURRENT_HEADER_DIR}/jsonschema/JsonSchemaChecker.h
	${CURRENT_SOURCE_DIR}/jsonschema/JsonSchemaChecker.cpp
)

if(ENABLE_QT5)
	qt5_use_modules(hyperion-utils Widgets)
endif()

target_link_libraries(hyperion-utils
	jsoncpp
	${QT_LIBRARIES})
