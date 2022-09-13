# Define the current source locations

if(ENABLE_EFFECTENGINE)
	# Include the python directory. Also include the parent (which is for example /usr/include)
	# which may be required when it is not includes by the (cross-) compiler by default.
	if (NOT CMAKE_VERSION VERSION_LESS "3.12")
		find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
		include_directories(${Python3_INCLUDE_DIRS} ${Python3_INCLUDE_DIRS}/..)
		add_compile_definitions(PYTHON_VERSION_MAJOR_MINOR=${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR})
	else()
		find_package (PythonLibs ${PYTHON_VERSION_STRING} EXACT) # Maps PythonLibs to the PythonInterp version of the main cmake
		include_directories(${PYTHON_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS}/..)
		add_definitions(-DPYTHON_VERSION_MAJOR_MINOR=${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR})
	endif()
endif()

SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/utils)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/utils)

FILE ( GLOB_RECURSE Utils_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

list(APPEND Utils_SOURCES "${CMAKE_SOURCE_DIR}/dependencies/include/oklab/ok_color.h")

if ( NOT ENABLE_PROFILER )
	LIST ( REMOVE_ITEM Utils_SOURCES ${CURRENT_HEADER_DIR}/Profiler.h ${CURRENT_SOURCE_DIR}/Profiler.cpp )
endif()

add_library(hyperion-utils
	${Utils_SOURCES}
)

target_link_libraries(hyperion-utils
	hyperion
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Gui
	Qt${QT_VERSION_MAJOR}::Network
)

if(ENABLE_EFFECTENGINE)
	target_link_libraries(hyperion-utils python)
endif()
