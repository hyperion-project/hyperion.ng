# Configure minimum CMAKE version
cmake_minimum_required(VERSION 2.8)

# Set the project name
project(hyperion-framebuffer)

# find QT
if(ENABLE_QT5)
	find_package(Qt5Widgets REQUIRED)
else()
	find_package(Qt4 REQUIRED QtCore QtGui QtNetwork )
endif()

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/protoserver
	${QT_INCLUDES}
	${PROTOBUF_INCLUDE_DIRS}
)

set(Hyperion_FB_QT_HEADERS
	FramebufferWrapper.h
)

set(Hyperion_FB_HEADERS
)

set(Hyperion_FB_SOURCES
	hyperion-framebuffer.cpp
	FramebufferWrapper.cpp
)

if(ENABLE_QT5)
	QT5_WRAP_CPP(Hyperion_FB_HEADERS_MOC ${Hyperion_FB_QT_HEADERS})
else()
	QT4_WRAP_CPP(Hyperion_FB_HEADERS_MOC ${Hyperion_FB_QT_HEADERS})
endif()

add_executable( ${PROJECT_NAME}
	${Hyperion_FB_HEADERS}
	${Hyperion_FB_SOURCES}
	${Hyperion_FB_HEADERS_MOC}
)

target_link_libraries( ${PROJECT_NAME}
	getoptPlusPlus
	blackborder
	hyperion-utils
	protoserver
	framebuffer-grabber
	pthread
)

if(ENABLE_QT5)
	qt5_use_modules(${PROJECT_NAME} Widgets Core Gui Network)
else()
	qt4_use_modules(${PROJECT_NAME} Core Gui Network )
endif()

install ( TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT ambilight )
