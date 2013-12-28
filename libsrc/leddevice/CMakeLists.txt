
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/leddevice)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/leddevice)

#add libusb and pthreads (required for the Lighpack usb device)
find_package(libusb-1.0 REQUIRED)
find_package(Threads REQUIRED)

include_directories(
	../../include/hidapi
	${LIBUSB_1_INCLUDE_DIRS}) # for Lightpack device

# Group the headers that go through the MOC compiler
SET(Leddevice_QT_HEADERS
		${CURRENT_SOURCE_DIR}/LedDeviceAdalight.h
)

SET(Leddevice_HEADERS
		${CURRENT_HEADER_DIR}/LedDevice.h
		${CURRENT_HEADER_DIR}/LedDeviceFactory.h

		${CURRENT_SOURCE_DIR}/LedRs232Device.h
		
		${CURRENT_SOURCE_DIR}/LedDeviceTest.h
		${CURRENT_SOURCE_DIR}/LedDeviceSedu.h
		${CURRENT_SOURCE_DIR}/LedDeviceWs2812b.h
		${CURRENT_SOURCE_DIR}/LedDeviceWs2811.h
		${CURRENT_SOURCE_DIR}/LedDeviceLightpack.h
		${CURRENT_SOURCE_DIR}/LedDevicePaintpack.h
		${CURRENT_SOURCE_DIR}/LedDeviceMultiLightpack.h
		${CURRENT_SOURCE_DIR}/LedDeviceWs2811.h
)

SET(Leddevice_SOURCES
		${CURRENT_SOURCE_DIR}/LedDeviceFactory.cpp

		${CURRENT_SOURCE_DIR}/LedDeviceAdalight.cpp
		${CURRENT_SOURCE_DIR}/LedRs232Device.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceSedu.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceTest.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceWs2811.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceWs2812b.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceAdalight.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceLightpack.cpp
		${CURRENT_SOURCE_DIR}/LedDevicePaintpack.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceMultiLightpack.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceWs2811.cpp
)

if(ENABLE_SPIDEV)
	SET(Leddevice_HEADERS
			${Leddevice_HEADERS}
			${CURRENT_SOURCE_DIR}/LedSpiDevice.h
			${CURRENT_SOURCE_DIR}/LedDeviceLpd6803.h
			${CURRENT_SOURCE_DIR}/LedDeviceLpd8806.h
			${CURRENT_SOURCE_DIR}/LedDeviceWs2801.h
	)
	SET(Leddevice_SOURCES
			${Leddevice_SOURCES}
			${CURRENT_SOURCE_DIR}/LedSpiDevice.cpp
			${CURRENT_SOURCE_DIR}/LedDeviceLpd6803.cpp
			${CURRENT_SOURCE_DIR}/LedDeviceLpd8806.cpp
			${CURRENT_SOURCE_DIR}/LedDeviceWs2801.cpp
	)
endif(ENABLE_SPIDEV)


QT4_WRAP_CPP(Leddevice_HEADERS_MOC ${Leddevice_QT_HEADERS})

add_library(leddevice
		${Leddevice_HEADERS}
		${Leddevice_QT_HEADERS}
		${Leddevice_HEADERS_MOC}
		${Leddevice_SOURCES}
)

target_link_libraries(leddevice
		hyperion-utils
		serialport
		${LIBUSB_1_LIBRARIES} #apt-get install libusb-1.0-0-dev
		${CMAKE_THREAD_LIBS_INIT}
		${QT_LIBRARIES}
)
if(APPLE)
	target_link_libraries(leddevice hidapi-mac)
else()
	target_link_libraries(leddevice hidapi-libusb)
endif()
