if(ENABLE_PROFILER)
	set(PROFILER ${CURRENT_HEADER_DIR}/Profiler.h ${CURRENT_SOURCE_DIR}/Profiler.cpp)
endif()

add_library(hyperion-utils
	# Global defines/signal sharing
	${CMAKE_SOURCE_DIR}/include/utils/global_defines.h
	${CMAKE_SOURCE_DIR}/include/utils/GlobalSignals.h
	# Error signal handler
	${CMAKE_SOURCE_DIR}/include/utils/ErrorManager.h
	# JSON Schema Checker
	${CMAKE_SOURCE_DIR}/include/utils/jsonschema/QJsonFactory.h
	${CMAKE_SOURCE_DIR}/include/utils/jsonschema/QJsonUtils.h
	${CMAKE_SOURCE_DIR}/include/utils/jsonschema/QJsonSchemaChecker.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/jsonschema/QJsonSchemaChecker.cpp
	# Color ARGB/BGR/RGB/RGBA/RGBW etc. structures
	${CMAKE_SOURCE_DIR}/include/utils/ColorArgb.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/ColorArgb.cpp
	${CMAKE_SOURCE_DIR}/include/utils/ColorBgr.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/ColorBgr.cpp
	${CMAKE_SOURCE_DIR}/include/utils/ColorRgb.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/ColorRgb.cpp
	${CMAKE_SOURCE_DIR}/include/utils/ColorRgba.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/ColorRgba.cpp
	${CMAKE_SOURCE_DIR}/include/utils/ColorRgbScalar.h
	${CMAKE_SOURCE_DIR}/include/utils/ColorRgbw.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/ColorRgbw.cpp
	# Image declaration
	${CMAKE_SOURCE_DIR}/include/utils/Image.h
	${CMAKE_SOURCE_DIR}/include/utils/ImageData.h
	# Image resampler
	${CMAKE_SOURCE_DIR}/include/utils/ImageResampler.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/ImageResampler.cpp
	# Color transformation (saturation/luminance) of RGB colors
	${CMAKE_SOURCE_DIR}/include/utils/ColorSys.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/ColorSys.cpp
	# Color transformation (saturation/value) of Okhsv colors
	${CMAKE_SOURCE_DIR}/include/utils/OkhsvTransform.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/OkhsvTransform.cpp
	# Signal handler
	${CMAKE_SOURCE_DIR}/include/utils/DefaultSignalHandler.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/DefaultSignalHandler.cpp
	# File utilities
	${CMAKE_SOURCE_DIR}/include/utils/FileUtils.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/FileUtils.cpp
	# JSON utilities
	${CMAKE_SOURCE_DIR}/include/utils/JsonUtils.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/JsonUtils.cpp
	# Logger
	${CMAKE_SOURCE_DIR}/include/utils/Logger.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/Logger.cpp
	# IP adress/Port checker
	${CMAKE_SOURCE_DIR}/include/utils/NetOrigin.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/NetOrigin.cpp
	${CMAKE_SOURCE_DIR}/include/utils/NetUtils.h
	# Process namespace (Hyperion restart)
	${CMAKE_SOURCE_DIR}/include/utils/Process.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/Process.cpp
	# Rgb single color adjustment/correction
	${CMAKE_SOURCE_DIR}/include/utils/RgbChannelAdjustment.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/RgbChannelAdjustment.cpp
	# Color conversion/transformation
	${CMAKE_SOURCE_DIR}/include/utils/RgbToRgbw.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/RgbToRgbw.cpp
	${CMAKE_SOURCE_DIR}/include/utils/RgbTransform.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/RgbTransform.cpp
	${CMAKE_SOURCE_DIR}/include/utils/KelvinToRgb.h	
	# System info class
	${CMAKE_SOURCE_DIR}/include/utils/SysInfo.h
	${CMAKE_SOURCE_DIR}/libsrc/utils/SysInfo.cpp
	# Grabber pixel formats enumeration
	${CMAKE_SOURCE_DIR}/include/utils/PixelFormat.h
	# Grabber playing modes enumeration
	${CMAKE_SOURCE_DIR}/include/utils/VideoMode.h
	# Grabber video standards enumeration
	${CMAKE_SOURCE_DIR}/include/utils/VideoStandard.h
	# SettingsManager utilities
	${CMAKE_SOURCE_DIR}/include/utils/settings.h
	# Qt string utilities
	${CMAKE_SOURCE_DIR}/include/utils/QStringUtils.h
	# QThread sleep class
	${CMAKE_SOURCE_DIR}/include/utils/Sleep.h
	# Wait event loop function
	${CMAKE_SOURCE_DIR}/include/utils/WaitTime.h
	# Tracking Shared objects' memory
	${CMAKE_SOURCE_DIR}/include/utils/TrackedMemory.h
	# Weak connection
	${CMAKE_SOURCE_DIR}/include/utils/WeakConnect.h
	# Semver namespace
	${CMAKE_SOURCE_DIR}/include/utils/version.hpp
	# Utility methods for Hyperion class
	${CMAKE_SOURCE_DIR}/include/utils/hyperion.h
	# Oklab color space
	${CMAKE_SOURCE_DIR}/dependencies/include/oklab/ok_color.h
	# Performance tester
	${PROFILER}
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
