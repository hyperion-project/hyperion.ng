if (NOT CMAKE_VERSION VERSION_LESS "3.12")
	find_package(Python COMPONENTS Interpreter Development REQUIRED)
else()
	find_package (PythonLibs ${PYTHON_VERSION_STRING} EXACT) # Maps PythonLibs to the PythonInterp version of the main cmake
endif()

# Include the python directory. Also include the parent (which is for example /usr/include)
# which may be required when it is not includes by the (cross-) compiler by default.
include_directories(${PYTHON_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS}/..)

# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/python)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/python)

FILE ( GLOB PYTHON_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(python
	${PYTHON_SOURCES}
)

target_link_libraries(python
	effectengine
	hyperion-utils
	${PYTHON_LIBRARIES}
)
