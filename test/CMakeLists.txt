
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

# Find the libPNG
find_package(PNG REQUIRED QUIET)

# Add additional includes dirs
include_directories(${PNG_INCLUDE_DIR})

#if(PNG_FOUND)

	add_executable(TestHyperionPng
			TestHyperionPng.cpp)

	target_link_libraries(TestHyperionPng
			hyperion-png)
#endif(PNG_FOUND)
