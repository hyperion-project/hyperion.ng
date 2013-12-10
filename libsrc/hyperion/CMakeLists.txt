
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/hyperion)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/hyperion)

#add libusb and pthreads (required for the Lighpack usb device)
find_package(libusb-1.0 REQUIRED)
find_package(Threads REQUIRED)

include_directories(
	../../include/hidapi
	${LIBUSB_1_INCLUDE_DIRS}) # for Lightpack device

# set the build options
option (LIBUDEV_REQUIRED "Add libudev to the build (needed on Ubuntu 13.10, but gives errors when added on the RPi) :-S" OFF)
message(STATUS "LIBUDEV_REQUIRED = " ${LIBUDEV_REQUIRED})

if (LIBUDEV_REQUIRED)
	find_package(UDev REQUIRED)
	include_directories(${UDEV_INCLUDE_DIR})
endif (LIBUDEV_REQUIRED)

# Group the headers that go through the MOC compiler
SET(Hyperion_QT_HEADERS
		${CURRENT_HEADER_DIR}/Hyperion.h

		${CURRENT_SOURCE_DIR}/LinearColorSmoothing.h
		${CURRENT_SOURCE_DIR}/device/LedDeviceAdalight.h
)

SET(Hyperion_HEADERS
		${CURRENT_HEADER_DIR}/ImageProcessor.h
		${CURRENT_HEADER_DIR}/ImageProcessorFactory.h
		${CURRENT_HEADER_DIR}/ImageToLedsMap.h
		${CURRENT_HEADER_DIR}/LedDevice.h
		${CURRENT_HEADER_DIR}/LedString.h
		${CURRENT_HEADER_DIR}/PriorityMuxer.h

		${CURRENT_HEADER_DIR}/BlackBorderDetector.h
		${CURRENT_HEADER_DIR}/BlackBorderProcessor.h

		${CURRENT_SOURCE_DIR}/MultiColorTransform.h

		${CURRENT_SOURCE_DIR}/device/LedSpiDevice.h
		${CURRENT_SOURCE_DIR}/device/LedRs232Device.h
		${CURRENT_SOURCE_DIR}/device/LedDeviceTest.h
		${CURRENT_SOURCE_DIR}/device/LedDeviceSedu.h
		${CURRENT_SOURCE_DIR}/device/LedDeviceWs2801.h
		${CURRENT_SOURCE_DIR}/device/LedDeviceWs2811.h
		${CURRENT_SOURCE_DIR}/device/LedDeviceLpd6803.h
		${CURRENT_SOURCE_DIR}/device/LedDeviceLpd8806.h
		${CURRENT_SOURCE_DIR}/device/LedDeviceLightpack.h
		${CURRENT_SOURCE_DIR}/device/LedDeviceMultiLightpack.h
)

SET(Hyperion_SOURCES
		${CURRENT_SOURCE_DIR}/Hyperion.cpp
		${CURRENT_SOURCE_DIR}/ImageProcessor.cpp
		${CURRENT_SOURCE_DIR}/ImageProcessorFactory.cpp
		${CURRENT_SOURCE_DIR}/LedString.cpp
		${CURRENT_SOURCE_DIR}/PriorityMuxer.cpp

		${CURRENT_SOURCE_DIR}/BlackBorderDetector.cpp
		${CURRENT_SOURCE_DIR}/BlackBorderProcessor.cpp
		${CURRENT_SOURCE_DIR}/ImageToLedsMap.cpp
		${CURRENT_SOURCE_DIR}/MultiColorTransform.cpp
		${CURRENT_SOURCE_DIR}/LinearColorSmoothing.cpp

		${CURRENT_SOURCE_DIR}/device/LedSpiDevice.cpp
		${CURRENT_SOURCE_DIR}/device/LedRs232Device.cpp
		${CURRENT_SOURCE_DIR}/device/LedDeviceSedu.cpp
		${CURRENT_SOURCE_DIR}/device/LedDeviceTest.cpp
		${CURRENT_SOURCE_DIR}/device/LedDeviceWs2801.cpp
		${CURRENT_SOURCE_DIR}/device/LedDeviceWs2811.cpp
		${CURRENT_SOURCE_DIR}/device/LedDeviceLpd6803.cpp
		${CURRENT_SOURCE_DIR}/device/LedDeviceLpd8806.cpp
		${CURRENT_SOURCE_DIR}/device/LedDeviceAdalight.cpp
		${CURRENT_SOURCE_DIR}/device/LedDeviceLightpack.cpp
		${CURRENT_SOURCE_DIR}/device/LedDeviceMultiLightpack.cpp
)

set(Hyperion_RESOURCES
		${CURRENT_SOURCE_DIR}/resource.qrc
)

QT4_WRAP_CPP(Hyperion_HEADERS_MOC ${Hyperion_QT_HEADERS})

qt4_add_resources(Hyperion_RESOURCES_RCC ${Hyperion_RESOURCES} OPTIONS "-no-compress")

add_library(hyperion
		${Hyperion_HEADERS}
		${Hyperion_QT_HEADERS}
		${Hyperion_HEADERS_MOC}
		${Hyperion_SOURCES}
		${Hyperion_RESOURCES_RCC}
)

target_link_libraries(hyperion
		hyperion-utils
		serialport
		${LIBUSB_1_LIBRARIES} #apt-get install libusb-1.0-0-dev
		${CMAKE_THREAD_LIBS_INIT}
		${QT_LIBRARIES}
)

if (LIBUDEV_REQUIRED)
	target_link_libraries(hyperion
			${UDEV_LIBRARIES} # apt-get install libudev-dev
	)
endif (LIBUDEV_REQUIRED)
