# Configure minimum CMAKE version
cmake_minimum_required(VERSION 2.8)

# Set the project name
project(hyperion-dispmanx)

# find QT
if(ENABLE_QT5)
	find_package(Qt5Widgets REQUIRED)
else()
	find_package(Qt4 REQUIRED QtCore QtGui QtNetwork )
endif()

# Find the BCM-package (VC control)
find_package(BCM REQUIRED)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/protoserver
	${QT_INCLUDES}
	${BCM_INCLUDE_DIRS}
	${PROTOBUF_INCLUDE_DIRS}
)

set(Hyperion_Dispmanx_QT_HEADERS
	DispmanxWrapper.h)

set(Hyperion_Dispmanx_HEADERS
)

set(Hyperion_Dispmanx_SOURCES
	hyperion-dispmanx.cpp
	DispmanxWrapper.cpp
)

if(ENABLE_QT5)
	QT5_WRAP_CPP(Hyperion_Dispmanx_HEADERS_MOC ${Hyperion_Dispmanx_QT_HEADERS})
else()
	QT4_WRAP_CPP(Hyperion_Dispmanx_HEADERS_MOC ${Hyperion_Dispmanx_QT_HEADERS})
endif()

add_executable( ${PROJECT_NAME}
	${Hyperion_Dispmanx_HEADERS}
	${Hyperion_Dispmanx_SOURCES}
	${Hyperion_Dispmanx_HEADERS_MOC}
)

target_link_libraries( ${PROJECT_NAME}
	getoptPlusPlus
	blackborder
	hyperion-utils
	protoserver
	dispmanx-grabber
	${Dispmanx_LIBRARIES}
	pthread
)

if(ENABLE_QT5)
	qt5_use_modules(${PROJECT_NAME} Widgets Core Gui Network)
else()
	qt4_use_modules(${PROJECT_NAME} Core Gui Network )
endif()

install ( TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT ambilight )
