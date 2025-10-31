add_library(hyperion
	# Authorization Manager
	${CMAKE_SOURCE_DIR}/include/hyperion/AuthManager.h
	${CMAKE_SOURCE_DIR}/libsrc/hyperion/AuthManager.cpp
	# Background Effect Handler
	${CMAKE_SOURCE_DIR}/include/hyperion/BGEffectHandler.h
	${CMAKE_SOURCE_DIR}/libsrc/hyperion/BGEffectHandler.cpp
	# Capture Control class
	${CMAKE_SOURCE_DIR}/include/hyperion/CaptureCont.h
	${CMAKE_SOURCE_DIR}/libsrc/hyperion/CaptureCont.cpp
	# Color Adjustment
	${CMAKE_SOURCE_DIR}/include/hyperion/ColorAdjustment.h
	# Component Register
	${CMAKE_SOURCE_DIR}/include/hyperion/ComponentRegister.h
	${CMAKE_SOURCE_DIR}/libsrc/hyperion/ComponentRegister.cpp
	# Grabber/Wrapper classes
	${CMAKE_SOURCE_DIR}/include/hyperion/Grabber.h
	${CMAKE_SOURCE_DIR}/libsrc/hyperion/Grabber.cpp
	${CMAKE_SOURCE_DIR}/include/hyperion/GrabberWrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/hyperion/GrabberWrapper.cpp
	# Hyperion + Resources
	${CMAKE_SOURCE_DIR}/include/hyperion/Hyperion.h
	${CMAKE_SOURCE_DIR}/libsrc/hyperion/Hyperion.cpp
	${CMAKE_SOURCE_DIR}/libsrc/hyperion/resource.qrc
	# Instance Manager
	${CMAKE_SOURCE_DIR}/include/hyperion/HyperionIManager.h
	${CMAKE_SOURCE_DIR}/libsrc/hyperion/HyperionIManager.cpp
	# Image Processor
	${CMAKE_SOURCE_DIR}/include/hyperion/ImageProcessor.h
	${CMAKE_SOURCE_DIR}/libsrc/hyperion/ImageProcessor.cpp
	# ImageToLedsMap class
	${CMAKE_SOURCE_DIR}/include/hyperion/ImageToLedsMap.h
	${CMAKE_SOURCE_DIR}/libsrc/hyperion/ImageToLedsMap.cpp
	# Led String
	${CMAKE_SOURCE_DIR}/include/hyperion/LedString.h
	${CMAKE_SOURCE_DIR}/libsrc/hyperion/LedString.cpp
	# Linear Color Smoothing
	${CMAKE_SOURCE_DIR}/include/hyperion/LinearColorSmoothing.h
	${CMAKE_SOURCE_DIR}/libsrc/hyperion/LinearColorSmoothing.cpp
	# Led Color Transform
	${CMAKE_SOURCE_DIR}/include/hyperion/MultiColorAdjustment.h
	${CMAKE_SOURCE_DIR}/libsrc/hyperion/MultiColorAdjustment.cpp
	# Priority Muxer
	${CMAKE_SOURCE_DIR}/include/hyperion/PriorityMuxer.h
	${CMAKE_SOURCE_DIR}/libsrc/hyperion/PriorityMuxer.cpp
	# Settings Manager
	${CMAKE_SOURCE_DIR}/include/hyperion/SettingsManager.h
	${CMAKE_SOURCE_DIR}/libsrc/hyperion/SettingsManager.cpp
)

target_link_libraries(hyperion
	blackborder
	events
	hyperion-utils
	leddevice
	database
)

if(ENABLE_BOBLIGHT_SERVER)
	target_link_libraries(hyperion boblightserver)
endif()

if(ENABLE_EFFECTENGINE)
	target_link_libraries(hyperion effectengine)
endif()

if(ENABLE_FLATBUF_SERVER)
	target_link_libraries(hyperion flatbufserver)
endif()

if(ENABLE_FORWARDER)
	target_link_libraries(hyperion forwarder)
endif()
