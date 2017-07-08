 # Needed for testing non-public components
include_directories(../libsrc)

find_package(Qt5Widgets REQUIRED)

if(ENABLE_SPIDEV)
	# Add the simple test executable 'TestSpi'
	add_executable(test_spi TestSpi.cpp)
	target_link_libraries(test_spi hyperion effectengine)

	add_executable(spidev_test spidev_test.c)

	add_executable(gpio2spi switchPinCtrl.c)
endif(ENABLE_SPIDEV)

add_executable(test_configfile
		TestConfigFile.cpp)
target_link_libraries(test_configfile
		hyperion
		effectengine
		)

add_executable(test_ImageRgb
		TestRgbImage.cpp)
target_link_libraries(test_ImageRgb
		hyperion-utils)

add_executable(test_image2ledsmap
		TestImage2LedsMap.cpp)
target_link_libraries(test_image2ledsmap
		hyperion
		effectengine
		)

if (ENABLE_DISPMANX)
	add_subdirectory(dispmanx2png)
endif (ENABLE_DISPMANX)

add_executable(test_blackborderdetector TestBlackBorderDetector.cpp)
target_link_libraries(test_blackborderdetector
		effectengine
 hyperion)

add_executable(test_blackborderprocessor
		TestBlackBorderProcessor.cpp)
target_link_libraries(test_blackborderprocessor
		hyperion
		effectengine
)

add_executable(test_qregexp TestQRegExp.cpp)
target_link_libraries(test_qregexp
		Qt5::Widgets)

add_executable(test_qtscreenshot TestQtScreenshot.cpp)
target_link_libraries(test_qtscreenshot
		Qt5::Widgets)

if(ENABLE_X11)
    # Find X11
    find_package(X11 REQUIRED)

    add_executable(test_x11performance TestX11Performance.cpp)
	target_link_libraries(test_x11performance ${X11_LIBRARIES} Qt5::Widgets)

endif(ENABLE_X11)
