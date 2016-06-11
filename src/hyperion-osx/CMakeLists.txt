# Configure minimum CMAKE version
cmake_minimum_required(VERSION 2.8)

# Set the project name
project(hyperion-osx)

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

set(Hyperion_OSX_QT_HEADERS
	OsxWrapper.h
)

set(Hyperion_OSX_HEADERS
)

set(Hyperion_OSX_SOURCES
	hyperion-osx.cpp
	OsxWrapper.cpp
)

if(ENABLE_QT5)
	QT5_WRAP_CPP(Hyperion_OSX_HEADERS_MOC ${Hyperion_OSX_QT_HEADERS})
else()
	QT4_WRAP_CPP(Hyperion_OSX_HEADERS_MOC ${Hyperion_OSX_QT_HEADERS})
endif()

add_executable( ${PROJECT_NAME}
	${Hyperion_OSX_HEADERS}
	${Hyperion_OSX_SOURCES}
	${Hyperion_OSX_HEADERS_MOC}
)

target_link_libraries( ${PROJECT_NAME}
	getoptPlusPlus
	blackborder
	hyperion-utils
	protoserver
	osx-grabber
	pthread
)

if(ENABLE_QT5)
	qt5_use_modules(${PROJECT_NAME} Widgets Core Gui Network)
else()
	qt4_use_modules(${PROJECT_NAME} Core Gui Network )
endif()

install ( TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT ambilight )

