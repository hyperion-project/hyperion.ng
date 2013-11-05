project(jsoncpp)

# define the current source/header path
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/dependencies/include/json)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/dependencies/build/jsoncpp)

add_library(jsoncpp
		${CURRENT_HEADER_DIR}/autolink.h
		${CURRENT_HEADER_DIR}/config.h
		${CURRENT_HEADER_DIR}/features.h
		${CURRENT_HEADER_DIR}/forwards.h
		${CURRENT_HEADER_DIR}/json.h
		${CURRENT_HEADER_DIR}/reader.h
		${CURRENT_HEADER_DIR}/value.h
		${CURRENT_HEADER_DIR}/writer.h

		${CURRENT_SOURCE_DIR}/json_batchallocator.h
		${CURRENT_SOURCE_DIR}/json_internalarray.inl
		${CURRENT_SOURCE_DIR}/json_internalmap.inl
		${CURRENT_SOURCE_DIR}/json_reader.cpp
		${CURRENT_SOURCE_DIR}/json_tool.h
		${CURRENT_SOURCE_DIR}/json_value.cpp
		${CURRENT_SOURCE_DIR}/json_valueiterator.inl
		${CURRENT_SOURCE_DIR}/json_writer.cpp)
