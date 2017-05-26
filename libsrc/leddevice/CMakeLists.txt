
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/leddevice)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/leddevice)

#add libusb and pthreads (required for the Lighpack usb device)
find_package(libusb-1.0 REQUIRED)
#find_package(Threads REQUIRED)

include_directories(
	${CMAKE_SOURCE_DIR}/include/hidapi
	${LIBUSB_1_INCLUDE_DIRS}
) # for Lightpack device

# Group the headers that go through the MOC compiler
SET(Leddevice_QT_HEADERS
	${CURRENT_HEADER_DIR}/LedDevice.h
	${CURRENT_SOURCE_DIR}/ProviderRs232.h
	${CURRENT_SOURCE_DIR}/LedDeviceAdalight.h
	${CURRENT_SOURCE_DIR}/LedDeviceAtmoOrb.h
	${CURRENT_SOURCE_DIR}/LedDevicePhilipsHue.h
	${CURRENT_SOURCE_DIR}/ProviderHID.h
	${CURRENT_SOURCE_DIR}/LedDeviceRawHID.h
	${CURRENT_SOURCE_DIR}/LedDeviceFadeCandy.h
)

SET(Leddevice_HEADERS
	${CURRENT_HEADER_DIR}/LedDeviceFactory.h

	${CURRENT_SOURCE_DIR}/LedDeviceLightpack.h
	${CURRENT_SOURCE_DIR}/LedDeviceMultiLightpack.h
	${CURRENT_SOURCE_DIR}/LedDevicePaintpack.h
	${CURRENT_SOURCE_DIR}/LedDevicePiBlaster.h
	${CURRENT_SOURCE_DIR}/LedDeviceSedu.h
	${CURRENT_SOURCE_DIR}/LedDeviceDMX.h
	${CURRENT_SOURCE_DIR}/LedDeviceFile.h
	${CURRENT_SOURCE_DIR}/LedDeviceUdpRaw.h
	${CURRENT_SOURCE_DIR}/LedDeviceUdpH801.h
	${CURRENT_SOURCE_DIR}/LedDeviceUdpE131.h
	${CURRENT_SOURCE_DIR}/LedDeviceUdpArtNet.h
	${CURRENT_SOURCE_DIR}/ProviderUdp.h
	${CURRENT_SOURCE_DIR}/LedDeviceHyperionUsbasp.h
	${CURRENT_SOURCE_DIR}/LedDeviceTpm2.h
	${CURRENT_SOURCE_DIR}/LedDeviceTpm2net.h
	${CURRENT_SOURCE_DIR}/LedDeviceAtmo.h
)

SET(Leddevice_SOURCES
	${CURRENT_SOURCE_DIR}/LedDevice.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceFactory.cpp

	${CURRENT_SOURCE_DIR}/ProviderRs232.cpp
	${CURRENT_SOURCE_DIR}/ProviderHID.cpp

	${CURRENT_SOURCE_DIR}/LedDeviceAdalight.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceAtmoOrb.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceRawHID.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceLightpack.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceMultiLightpack.cpp
	${CURRENT_SOURCE_DIR}/LedDevicePaintpack.cpp
	${CURRENT_SOURCE_DIR}/LedDevicePiBlaster.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceSedu.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceDMX.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceFile.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceFadeCandy.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceUdpRaw.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceUdpH801.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceUdpE131.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceUdpArtNet.cpp
	${CURRENT_SOURCE_DIR}/ProviderUdp.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceHyperionUsbasp.cpp
	${CURRENT_SOURCE_DIR}/LedDevicePhilipsHue.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceTpm2.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceTpm2net.cpp
	${CURRENT_SOURCE_DIR}/LedDeviceAtmo.cpp
)

if(ENABLE_SPIDEV)
	SET(Leddevice_HEADERS
		${Leddevice_HEADERS}
		${CURRENT_SOURCE_DIR}/ProviderSpi.h
		${CURRENT_SOURCE_DIR}/LedDeviceLpd6803.h
		${CURRENT_SOURCE_DIR}/LedDeviceLpd8806.h
		${CURRENT_SOURCE_DIR}/LedDeviceP9813.h
		${CURRENT_SOURCE_DIR}/LedDeviceWs2801.h
		${CURRENT_SOURCE_DIR}/LedDeviceWs2812SPI.h
		${CURRENT_SOURCE_DIR}/LedDeviceSk6822SPI.h
		${CURRENT_SOURCE_DIR}/LedDeviceSk6812SPI.h
		${CURRENT_SOURCE_DIR}/LedDeviceAPA102.h
	)
	SET(Leddevice_SOURCES
		${Leddevice_SOURCES}
		${CURRENT_SOURCE_DIR}/ProviderSpi.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceLpd6803.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceLpd8806.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceP9813.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceWs2801.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceWs2812SPI.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceSk6822SPI.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceSk6812SPI.cpp
		${CURRENT_SOURCE_DIR}/LedDeviceAPA102.cpp
	)
endif()

if(ENABLE_WS281XPWM)
	include_directories(../../dependencies/external/rpi_ws281x)
	SET(Leddevice_HEADERS
		${Leddevice_HEADERS}
		${CURRENT_SOURCE_DIR}/LedDeviceWS281x.h
		)
	SET(Leddevice_SOURCES
		${Leddevice_SOURCES}
		${CURRENT_SOURCE_DIR}/LedDeviceWS281x.cpp
	)
endif()

if(ENABLE_TINKERFORGE)
	SET(Leddevice_HEADERS
		${Leddevice_HEADERS}
		${CURRENT_SOURCE_DIR}/LedDeviceTinkerforge.h
	)
	SET(Leddevice_SOURCES
		${Leddevice_SOURCES}
		${CURRENT_SOURCE_DIR}/LedDeviceTinkerforge.cpp
	)
endif()

set(LedDevice_RESOURCES
	${CURRENT_SOURCE_DIR}/LedDeviceSchemas.qrc
)

QT5_WRAP_CPP(Leddevice_HEADERS_MOC ${Leddevice_QT_HEADERS})
qt5_add_resources(LedDevice_RESOURCES_RCC ${LedDevice_RESOURCES} OPTIONS "-no-compress")



add_library(leddevice
	${Leddevice_HEADERS}
	${Leddevice_QT_HEADERS}
	${Leddevice_HEADERS_MOC}
	${Leddevice_SOURCES}
	${LedDevice_RESOURCES}
	${LedDevice_RESOURCES_RCC}
)

qt5_use_modules(leddevice Network SerialPort)

target_link_libraries(leddevice
	hyperion-utils
	${LIBUSB_1_LIBRARIES} #apt-get install libusb-1.0-0-dev
	${CMAKE_THREAD_LIBS_INIT}
	${QT_LIBRARIES}
)

if(ENABLE_TINKERFORGE)
	target_link_libraries(leddevice tinkerforge)
endif()

if(ENABLE_WS281XPWM)
	target_link_libraries(leddevice ws281x)
endif()

if(APPLE)
	target_link_libraries(leddevice hidapi-mac)
else()
	target_link_libraries(leddevice hidapi-libusb)
endif()
