
find_package(PythonLibs REQUIRED)
#OpenElec uses 2.7, if you want to compile for OpenElec require 2.7
#find_package(PythonLibs 2.7 REQUIRED)

# Include the python directory. Also include the parent (which is for example /usr/include)
# which may be required when it is not includes by the (cross-) compiler by default.
include_directories(${PYTHON_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS}/..)

# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/effectengine)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/effectengine)

# Group the headers that go through the MOC compiler
SET(EffectEngineQT_HEADERS
	${CURRENT_HEADER_DIR}/EffectEngine.h
	${CURRENT_SOURCE_DIR}/Effect.h
)

SET(EffectEngineHEADERS
	${CURRENT_HEADER_DIR}/EffectDefinition.h
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

QT5_WRAP_CPP(EffectEngineHEADERS_MOC ${EffectEngineQT_HEADERS})
qt5_add_resources(EffectEngine_RESOURCES_RCC ${EffectEngine_RESOURCES} ) # OPTIONS "-no-compress"

add_library(effectengine
	${EffectEngineHEADERS}
	${EffectEngineQT_HEADERS}
	${EffectEngineHEADERS_MOC}
	${EffectEngine_RESOURCES_RCC}
	${EffectEngineSOURCES}
)

qt5_use_modules(effectengine Core Gui)

target_link_libraries(effectengine
	hyperion
	${QT_LIBRARIES}
	${PYTHON_LIBRARIES})
