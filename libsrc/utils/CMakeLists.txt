# Define the current source locations

SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/utils)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/utils)

SET(Utils_QT_HEADERS
	${CURRENT_HEADER_DIR}/Logger.h
	${CURRENT_HEADER_DIR}/Stats.h
	${CURRENT_HEADER_DIR}/JsonProcessor.h
)

SET(Utils_HEADERS
	${CURRENT_HEADER_DIR}/ColorBgr.h
	${CURRENT_HEADER_DIR}/ColorRgb.h
	${CURRENT_HEADER_DIR}/ColorRgba.h
	${CURRENT_HEADER_DIR}/ColorRgbw.h
	${CURRENT_HEADER_DIR}/Image.h
	${CURRENT_HEADER_DIR}/Sleep.h
	${CURRENT_HEADER_DIR}/FileUtils.h
	${CURRENT_HEADER_DIR}/Process.h
	${CURRENT_HEADER_DIR}/PixelFormat.h
	${CURRENT_HEADER_DIR}/VideoMode.h
	${CURRENT_HEADER_DIR}/ImageResampler.h
	${CURRENT_HEADER_DIR}/RgbTransform.h
	${CURRENT_HEADER_DIR}/ColorSys.h
	${CURRENT_HEADER_DIR}/RgbChannelAdjustment.h
	${CURRENT_HEADER_DIR}/RgbToRgbw.h
	${CURRENT_HEADER_DIR}/jsonschema/QJsonFactory.h
	${CURRENT_HEADER_DIR}/jsonschema/QJsonSchemaChecker.h
	${CURRENT_HEADER_DIR}/global_defines.h
	${CURRENT_HEADER_DIR}/SysInfo.h
)

SET(Utils_SOURCES
	${CURRENT_SOURCE_DIR}/ColorArgb.cpp
	${CURRENT_SOURCE_DIR}/ColorBgr.cpp
	${CURRENT_SOURCE_DIR}/ColorRgb.cpp
	${CURRENT_SOURCE_DIR}/ColorRgba.cpp
	${CURRENT_SOURCE_DIR}/ColorRgbw.cpp
	${CURRENT_SOURCE_DIR}/FileUtils.cpp
	${CURRENT_SOURCE_DIR}/Process.cpp
	${CURRENT_SOURCE_DIR}/Logger.cpp
	${CURRENT_SOURCE_DIR}/ImageResampler.cpp
	${CURRENT_SOURCE_DIR}/ColorSys.cpp
	${CURRENT_SOURCE_DIR}/RgbChannelAdjustment.cpp
	${CURRENT_SOURCE_DIR}/RgbTransform.cpp
	${CURRENT_SOURCE_DIR}/RgbToRgbw.cpp
	${CURRENT_SOURCE_DIR}/SysInfo.cpp
	${CURRENT_SOURCE_DIR}/Stats.cpp
	${CURRENT_SOURCE_DIR}/JsonProcessor.cpp
	${CURRENT_SOURCE_DIR}/jsonschema/QJsonSchemaChecker.cpp
)

if ( ENABLE_PROFILER )
	SET ( PROFILER_SOURCE ${CURRENT_HEADER_DIR}/Profiler.h ${CURRENT_SOURCE_DIR}/Profiler.cpp )
endif()

qt5_wrap_cpp(Utils_HEADERS_MOC ${Utils_QT_HEADERS})


add_library(hyperion-utils
	${Utils_QT_HEADERS}
	${Utils_HEADERS_MOC}
	${Utils_HEADERS}
	${Utils_SOURCES}
	${PROFILER_SOURCE}
)

qt5_use_modules(hyperion-utils Core Gui Network)

target_link_libraries(hyperion-utils
	${QT_LIBRARIES})
