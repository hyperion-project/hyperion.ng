add_library(python
	${CMAKE_SOURCE_DIR}/include/python/PythonInit.h
	${CMAKE_SOURCE_DIR}/include/python/PythonProgram.h
	${CMAKE_SOURCE_DIR}/include/python/PythonUtils.h
	${CMAKE_SOURCE_DIR}/libsrc/python/PythonInit.cpp
	${CMAKE_SOURCE_DIR}/libsrc/python/PythonProgram.cpp
)

if(CMAKE_VERSION VERSION_LESS "3.12")
	find_package(PythonLibs ${PYTHON_VERSION_STRING} EXACT REQUIRED)
else()
	find_package(Python3 COMPONENTS Interpreter Development QUIET REQUIRED)
	set(PYTHON_VERSION_STRING ${Python3_VERSION})
	set(PYTHON_VERSION_MAJOR ${Python3_VERSION_MAJOR})
	set(PYTHON_VERSION_MINOR ${Python3_VERSION_MINOR})
	set(Python_STDLIB ${Python3_STDLIB})
endif()

define_property(TARGET PROPERTY PYTHON_VERSION_PROPERTY
	BRIEF_DOCS "Python version property"
	FULL_DOCS "Python version property"
)

define_property(TARGET PROPERTY PYTHON_STDLIB_LOCATION_PROPERTY
	BRIEF_DOCS "Python stdlib location property"
	FULL_DOCS "Python stdlib location property"
)

set_target_properties(python PROPERTIES
	PYTHON_VERSION_PROPERTY ${PYTHON_VERSION_STRING}
	PYTHON_STDLIB_LOCATION_PROPERTY ${Python_STDLIB}
)

target_link_libraries(python
	effectengine
	hyperion-utils
	${Python3_LIBRARIES}
	${PYTHON_LIBRARIES}
)

target_include_directories(python PUBLIC
	${Python3_INCLUDE_DIRS} ${Python3_INCLUDE_DIRS}/..
	${PYTHON_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS}/..
)

target_compile_definitions(python PRIVATE
	PYTHON_VERSION_MAJOR=${PYTHON_VERSION_MAJOR}
	PYTHON_VERSION_MINOR=${PYTHON_VERSION_MINOR}
)
