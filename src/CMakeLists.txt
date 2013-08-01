
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

add_executable(boblight-dispmanx
		boblight-dispmanx.cpp
		flagmanager.h
		flagmanager.cpp
		flagmanager-dispmanx.h
		flagmanager-dispmanx.cpp
		grabber-dispmanx.h
		grabber-dispmanx.cpp
		misc.h
		misc.cpp
		timer.h
		timer.cpp
		timeutils.h
		timeutils.cpp)

FIND_PATH(VC_INCLUDE_DIR
		bcm_host.h
		/usr/include
		/usr/local/include
		/opt/vc/include)
SET(VC_INCLUDE_DIRS
		${VC_INCLUDE_DIR}
		${VC_INCLUDE_DIR}/interface/vcos/pthreads
		${VC_INCLUDE_DIR}/interface/vmcs_host/linux)
FIND_LIBRARY(VC_LIBS
		NAMES bcm_host
		PATHS /usr/lib /usr/local/lib /opt/vc/lib)
message("{VC_LIBS} = ${VC_LIBS}")

#SET(VC_LIBS
#	/home/pi/.xbmc-current/xbmc-bin/lib/xbmc/system/libbcm_host.so)
#	/home/pi/.xbmc-current/xbmc-bin/lib/xbmc/system/libGLESv2.so
#	/home/pi/.xbmc-current/xbmc-bin/lib/xbmc/system/libEGL.so
#	/home/pi/.xbmc-current/xbmc-bin/lib/xbmc/system/libopenmaxil.so
#	/home/pi/.xbmc-current/xbmc-bin/lib/xbmc/system/libvcos.so
#	/home/pi/.xbmc-current/xbmc-bin/lib/xbmc/system/libvchiq_arm.so)

include_directories(${VC_INCLUDE_DIRS})

target_link_libraries(boblight-dispmanx
#		hyperion-png
		bob2hyperion
		${VC_LIBS})
