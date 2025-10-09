file(GLOB effectFiles RELATIVE ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/effects/*)
set(HYPERION_EFFECTS_RES "")
foreach(f ${effectFiles})
	get_filename_component(fname ${f} NAME)
	set(HYPERION_EFFECTS_RES "${HYPERION_EFFECTS_RES}\n\t\t<file alias=\"/effects/${fname}\">${f}</file>")
endforeach()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/EffectEngine.qrc.in ${CMAKE_BINARY_DIR}/EffectEngine.qrc)

add_library(effectengine
	${CMAKE_BINARY_DIR}/EffectEngine.qrc
	${CMAKE_SOURCE_DIR}/include/effectengine/ActiveEffectDefinition.h
	${CMAKE_SOURCE_DIR}/include/effectengine/Effect.h
	${CMAKE_SOURCE_DIR}/include/effectengine/EffectDefinition.h
	${CMAKE_SOURCE_DIR}/include/effectengine/EffectEngine.h
	${CMAKE_SOURCE_DIR}/include/effectengine/EffectFileHandler.h
	${CMAKE_SOURCE_DIR}/include/effectengine/EffectModule.h
	${CMAKE_SOURCE_DIR}/include/effectengine/EffectSchema.h
	${CMAKE_SOURCE_DIR}/libsrc/effectengine/Effect.cpp
	${CMAKE_SOURCE_DIR}/libsrc/effectengine/EffectEngine.cpp
	${CMAKE_SOURCE_DIR}/libsrc/effectengine/EffectFileHandler.cpp
	${CMAKE_SOURCE_DIR}/libsrc/effectengine/EffectModule.cpp
)

target_link_libraries(effectengine
	python
	hyperion
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Gui
)

if(NOT CMAKE_VERSION VERSION_LESS "3.15")
	set_target_properties(effectengine PROPERTIES
		ADDITIONAL_CLEAN_FILES ${CMAKE_BINARY_DIR}/EffectEngine.qrc
	)
endif()
