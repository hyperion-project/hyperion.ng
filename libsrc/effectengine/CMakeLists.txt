
find_package(PythonLibs REQUIRED)

include_directories(${PYTHON_INCLUDE_DIRS})

# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/effectengine)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/effectengine)

# Group the headers that go through the MOC compiler
SET(EffectEngineQT_HEADERS
		${CURRENT_HEADER_DIR}/EffectEngine.h
		${CURRENT_SOURCE_DIR}/Effect.h
)

SET(EffectEngineHEADERS
)

SET(EffectEngineSOURCES
		${CURRENT_SOURCE_DIR}/EffectEngine.cpp
		${CURRENT_SOURCE_DIR}/Effect.cpp
)

QT4_WRAP_CPP(EffectEngineHEADERS_MOC ${EffectEngineQT_HEADERS})

add_library(effectengine
		${EffectEngineHEADERS}
		${EffectEngineQT_HEADERS}
		${EffectEngineHEADERS_MOC}
		${EffectEngineSOURCES}
)

target_link_libraries(effectengine
		hyperion
		${QT_LIBRARIES}
		${PYTHON_LIBRARIES})
