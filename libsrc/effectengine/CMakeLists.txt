find_package(PythonLibs 3.4 REQUIRED)

# Include the python directory. Also include the parent (which is for example /usr/include)
# which may be required when it is not includes by the (cross-) compiler by default.
include_directories(${PYTHON_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS}/..)

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
	Qt5::Core
	Qt5::Gui
	${PYTHON_LIBRARIES}
)
