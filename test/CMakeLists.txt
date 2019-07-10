# Needed for testing non-public components
include_directories(../libsrc)

find_package(Qt5Widgets REQUIRED)

MACRO (link_to_hyperion TARGET)
	target_link_libraries( ${TARGET} blackborder leddevice jsonserver hyperion-utils hyperion effectengine )
ENDMACRO()

if(ENABLE_SPIDEV)
	# Add the simple test executable 'TestSpi'
	add_executable(test_spi TestSpi.cpp)
	target_link_libraries( test_spi leddevice hyperion-utils hyperion )
	add_executable(spidev_test spidev_test.c)
	add_executable(gpio2spi switchPinCtrl.c)
endif(ENABLE_SPIDEV)

add_executable(test_configfile TestConfigFile.cpp)
link_to_hyperion(test_configfile)

add_executable(test_ImageRgb TestRgbImage.cpp)
link_to_hyperion(test_ImageRgb)

add_executable(test_blackborderdetector TestBlackBorderDetector.cpp)
link_to_hyperion(test_blackborderdetector)

add_executable(test_qregexp TestQRegExp.cpp)
target_link_libraries(test_qregexp Qt5::Widgets)

add_executable(test_qtscreenshot TestQtScreenshot.cpp)
target_link_libraries(test_qtscreenshot Qt5::Widgets)

if(ENABLE_X11)
	find_package(X11 REQUIRED)
	add_executable(test_x11performance TestX11Performance.cpp)
	target_link_libraries(test_x11performance ${X11_LIBRARIES} Qt5::Widgets)
endif(ENABLE_X11)

######### These tests are broken. May they fix someone ##########

# add_executable(test_image2ledsmap TestImage2LedsMap.cpp)
# link_to_hyperion(test_image2ledsmap)

# if (ENABLE_DISPMANX)
#	add_subdirectory(dispmanx2png)
# endif (ENABLE_DISPMANX)

# add_executable(test_blackborderprocessor TestBlackBorderProcessor.cpp)
# link_to_hyperion(test_blackborderprocessor)

###################################################
