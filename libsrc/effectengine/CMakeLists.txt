if (NOT CMAKE_VERSION VERSION_LESS "3.12")
	find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
else()
	find_package (PythonLibs ${PYTHON_VERSION_STRING} EXACT) # Maps PythonLibs to the PythonInterp version of the main cmake
endif()

# Include the python directory. Also include the parent (which is for example /usr/include)
# which may be required when it is not includes by the (cross-) compiler by default.
if (NOT CMAKE_VERSION VERSION_LESS "3.12")
	include_directories(${Python3_INCLUDE_DIRS} ${Python3_INCLUDE_DIRS}/..)
else()
	include_directories(${PYTHON_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS}/..)
endif()

# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/effectengine)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/effectengine)

FILE ( GLOB EffectEngineSOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )
FILE ( GLOB effectFiles RELATIVE ${CMAKE_BINARY_DIR}  ${CMAKE_SOURCE_DIR}/effects/* )

SET ( HYPERION_EFFECTS_RES "")
FOREACH( f ${effectFiles} )
	GET_FILENAME_COMPONENT(fname ${f} NAME)
	SET(HYPERION_EFFECTS_RES "${HYPERION_EFFECTS_RES}\n\t\t<file alias=\"/effects/${fname}\">${f}</file>")
ENDFOREACH()
CONFIGURE_FILE(${CURRENT_SOURCE_DIR}/EffectEngine.qrc.in ${CMAKE_BINARY_DIR}/EffectEngine.qrc )

SET(EffectEngine_RESOURCES ${CMAKE_BINARY_DIR}/EffectEngine.qrc)

add_library(effectengine
	${EffectEngine_RESOURCES}
	${EffectEngineSOURCES}
)

target_link_libraries(effectengine
	hyperion
	python
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Gui
)

if (NOT CMAKE_VERSION VERSION_LESS "3.12")
	target_link_libraries( effectengine ${Python3_LIBRARIES} )
else()
	target_link_libraries( effectengine ${PYTHON_LIBRARIES} )
endif()
