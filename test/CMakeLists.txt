# Needed for testing non-public components
include_directories(../libsrc)

# Add the simple test executable 'TestSpi'
add_executable(TestSpi
		TestSpi.cpp)

target_link_libraries(TestSpi
		hyperion
		hyperion-utils)

add_executable(TestConfigFile
		TestConfigFile.cpp)

target_link_libraries(TestConfigFile
		hyperion-utils
		hyperion)

add_executable(Test2BobLight
		Test2BobLight.cpp)

target_link_libraries(Test2BobLight
		bob2hyperion)

add_executable(TestRgbImage
		TestRgbImage.cpp)
target_link_libraries(TestRgbImage
		hyperion-utils)

add_executable(TestColorTransform
    TestColorTransform.cpp)
target_link_libraries(TestColorTransform
    hyperion)

# Find the libPNG
find_package(PNG REQUIRED QUIET)

#if(PNG_FOUND)
    # Add additional includes dirs
    include_directories(${PNG_INCLUDE_DIR})

    add_executable(TestHyperionPng
			TestHyperionPng.cpp)

	target_link_libraries(TestHyperionPng
			hyperion-png)
#endif(PNG_FOUND)
