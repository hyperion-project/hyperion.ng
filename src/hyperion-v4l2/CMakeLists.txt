cmake_minimum_required(VERSION 2.8)

project(hyperion-v4l2)

# find Qt
if(ENABLE_QT5)
	find_package(Qt5Widgets REQUIRED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}    ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
#	set(CMAKE_CXX_FLAGS "-fPIC")
else()
	find_package(Qt4 REQUIRED QtCore QtGui QtNetwork)
endif()


include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/protoserver
	${PROTOBUF_INCLUDE_DIRS}
	${QT_INCLUDES}
)

set(Hyperion_V4L2_QT_HEADERS
	ScreenshotHandler.h
)

set(Hyperion_V4L2_HEADERS
	VideoStandardParameter.h
	PixelFormatParameter.h
)

set(Hyperion_V4L2_SOURCES
	hyperion-v4l2.cpp
	ScreenshotHandler.cpp
)
if(ENABLE_QT5)
	QT5_WRAP_CPP(Hyperion_V4L2_MOC_SOURCES ${Hyperion_V4L2_QT_HEADERS})
else()
	QT4_WRAP_CPP(Hyperion_V4L2_MOC_SOURCES ${Hyperion_V4L2_QT_HEADERS})
endif()

add_executable(${PROJECT_NAME}
	${Hyperion_V4L2_HEADERS}
	${Hyperion_V4L2_SOURCES}
	${Hyperion_V4L2_QT_HEADERS}
	${Hyperion_V4L2_MOC_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
	v4l2-grabber
	getoptPlusPlus
	blackborder
	hyperion-utils
	protoserver
	pthread
	${QT_LIBRARIES}
)

if(ENABLE_QT5)
	qt5_use_modules(${PROJECT_NAME} Widgets Core Gui Network)
else()
	qt4_use_modules(${PROJECT_NAME} Core Gui Network )
endif()

install ( TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT ambilight )
