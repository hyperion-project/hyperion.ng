# Needed for testing non-public components
include_directories(../libsrc)

# Add the simple test executable 'TestSpi'
add_executable(TestSpi
		TestSpi.cpp)
target_link_libraries(TestSpi
		hyperion)


add_executable(TestConfigFile
		TestConfigFile.cpp)
target_link_libraries(TestConfigFile
		hyperion)

add_executable(TestRgbImage
		TestRgbImage.cpp)
target_link_libraries(TestRgbImage
		hyperion-utils)

add_executable(TestColorTransform
	TestColorTransform.cpp)
target_link_libraries(TestColorTransform
	hyperion)

add_executable(TestImage2LedsMap
		TestImage2LedsMap.cpp)
target_link_libraries(TestImage2LedsMap
		hyperion)

add_subdirectory(dispmanx2png)
