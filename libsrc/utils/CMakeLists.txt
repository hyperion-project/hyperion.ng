
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/utils)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/utils)

add_library(hyperion-utils
		${CURRENT_HEADER_DIR}/RgbColor.h
		${CURRENT_HEADER_DIR}/RgbImage.h
		${CURRENT_HEADER_DIR}/ColorTransform.h
		${CURRENT_HEADER_DIR}/HsvTransform.h

		${CURRENT_SOURCE_DIR}/RgbColor.cpp
		${CURRENT_SOURCE_DIR}/RgbImage.cpp
		${CURRENT_SOURCE_DIR}/ColorTransform.cpp
		${CURRENT_SOURCE_DIR}/HsvTransform.cpp

		${CURRENT_HEADER_DIR}/jsonschema/JsonFactory.h
		${CURRENT_HEADER_DIR}/jsonschema/JsonSchemaChecker.h
		${CURRENT_SOURCE_DIR}/jsonschema/JsonSchemaChecker.cpp
)

target_link_libraries(hyperion-utils
		jsoncpp)
