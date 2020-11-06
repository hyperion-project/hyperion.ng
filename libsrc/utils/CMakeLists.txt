# Define the current source locations

# Include the python directory. Also include the parent (which is for example /usr/include)
# which may be required when it is not includes by the (cross-) compiler by default.
if (NOT CMAKE_VERSION VERSION_LESS "3.12")
        include_directories(${Python3_INCLUDE_DIRS} ${Python3_INCLUDE_DIRS}/..)
else()
        include_directories(${PYTHON_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS}/..)
endif()

SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/utils)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/utils)

FILE ( GLOB_RECURSE Utils_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

if ( NOT ENABLE_PROFILER )
	LIST ( REMOVE_ITEM Utils_SOURCES ${CURRENT_HEADER_DIR}/Profiler.h ${CURRENT_SOURCE_DIR}/Profiler.cpp )
endif()

add_library(hyperion-utils
	${Utils_SOURCES}
)

target_link_libraries(hyperion-utils
	hyperion
	python
	Qt5::Core
	Qt5::Gui
	Qt5::Network
)
