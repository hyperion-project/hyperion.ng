set(Python_ADDITIONAL_VERSIONS 2.7.12)
find_package(PythonLibs REQUIRED)

message( STATUS "PYTHON VERSIONS: ${PYTHONLIBS_VERSION_STRING}" )

# Include the python directory. Also include the parent (which is for example /usr/include)
# which may be required when it is not includes by the (cross-) compiler by default.
include_directories(${PYTHON_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS}/..)

# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/effectengine)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/effectengine)

SET(EffectEngineHEADERS
	${CURRENT_HEADER_DIR}/EffectDefinition.h
	${CURRENT_HEADER_DIR}/EffectEngine.h
	${CURRENT_SOURCE_DIR}/Effect.h
)

SET(EffectEngineSOURCES
	${CURRENT_SOURCE_DIR}/EffectEngine.cpp
	${CURRENT_SOURCE_DIR}/Effect.cpp
)


FILE ( GLOB effectFiles RELATIVE ${CMAKE_BINARY_DIR}  ${CMAKE_SOURCE_DIR}/effects/* )
SET ( HYPERION_EFFECTS_RES "")
FOREACH( f ${effectFiles} )
	GET_FILENAME_COMPONENT(fname ${f} NAME)
	SET(HYPERION_EFFECTS_RES "${HYPERION_EFFECTS_RES}\n\t\t<file alias=\"/effects/${fname}\">${f}</file>")
ENDFOREACH()
CONFIGURE_FILE(${CURRENT_SOURCE_DIR}/EffectEngine.qrc.in ${CMAKE_BINARY_DIR}/EffectEngine.qrc )

SET(EffectEngine_RESOURCES ${CMAKE_BINARY_DIR}/EffectEngine.qrc)

qt5_add_resources(EffectEngine_RESOURCES_RCC ${EffectEngine_RESOURCES} ) # OPTIONS "-no-compress"

add_library(effectengine
	${EffectEngineHEADERS}
	${EffectEngine_RESOURCES_RCC}
	${EffectEngineSOURCES}
)

target_link_libraries(effectengine
	hyperion
	Qt5::Core
	Qt5::Gui
	${PYTHON_LIBRARIES})
