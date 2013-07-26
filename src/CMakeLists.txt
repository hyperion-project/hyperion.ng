
# Find the libPNG
find_package(PNG REQUIRED QUIET)

# Add additional includes dirs
include_directories(${PNG_INCLUDE_DIR})

# Add the simple test executable 'TestSpi'
add_executable(TestSpi
		TestSpi.cpp)

target_link_libraries(TestSpi
		hyperion
		hyperion-utils)

add_executable(TestHyperionPng
		TestHyperionPng.cpp)

target_link_libraries(TestHyperionPng
		hyperion-png)

add_executable(WriteConfig
		WriteConfig.cpp)

add_executable(TestConfigFile
		TestConfigFile.cpp)

target_link_libraries(TestConfigFile
		hyperion-utils
		hyperion)

add_executable(ViewPng
		ViewPng.cpp)

target_link_libraries(ViewPng
		hyperion
		hyperion-utils
		${PNG_LIBRARIES})

add_executable(Test2BobLight
		Test2BobLight.cpp)

target_link_libraries(Test2BobLight
		bob2hyperion)
