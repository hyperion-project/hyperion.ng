
find_package(PythonLibs REQUIRED)

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


set(EffectEngine_RESOURCES ${CURRENT_SOURCE_DIR}/EffectEngine.qrc)

QT4_WRAP_CPP(EffectEngineHEADERS_MOC ${EffectEngineQT_HEADERS})

qt4_add_resources(EffectEngine_RESOURCES_RCC ${EffectEngine_RESOURCES} OPTIONS "-no-compress")

add_library(effectengine
		${EffectEngineHEADERS}
		${EffectEngineQT_HEADERS}
		${EffectEngineHEADERS_MOC}
		${EffectEngine_RESOURCES_RCC}
		${EffectEngineSOURCES}
)

target_link_libraries(effectengine
		hyperion
		jsoncpp
		${QT_LIBRARIES}
		${PYTHON_LIBRARIES})
