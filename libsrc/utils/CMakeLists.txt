
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/utils)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/utils)

add_library(hyperion-utils
		${CURRENT_HEADER_DIR}/ColorArgb.h
		${CURRENT_SOURCE_DIR}/ColorArgb.cpp
		${CURRENT_HEADER_DIR}/ColorRgb.h
		${CURRENT_SOURCE_DIR}/ColorRgb.cpp
		${CURRENT_HEADER_DIR}/ColorRgba.h
		${CURRENT_SOURCE_DIR}/ColorRgba.cpp
		${CURRENT_HEADER_DIR}/Image.h
		${CURRENT_HEADER_DIR}/Sleep.h

		${CURRENT_HEADER_DIR}/PixelFormat.h
		${CURRENT_HEADER_DIR}/VideoMode.h

		${CURRENT_HEADER_DIR}/ImageResampler.h
		${CURRENT_SOURCE_DIR}/ImageResampler.cpp

		${CURRENT_HEADER_DIR}/HsvTransform.h
		${CURRENT_SOURCE_DIR}/HsvTransform.cpp
		${CURRENT_HEADER_DIR}/RgbChannelTransform.h
		${CURRENT_SOURCE_DIR}/RgbChannelTransform.cpp

		${CURRENT_HEADER_DIR}/jsonschema/JsonFactory.h
		${CURRENT_HEADER_DIR}/jsonschema/JsonSchemaChecker.h
		${CURRENT_SOURCE_DIR}/jsonschema/JsonSchemaChecker.cpp
)

target_link_libraries(hyperion-utils
		jsoncpp)
