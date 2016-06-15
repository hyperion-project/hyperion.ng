# Configure minimum CMAKE version
cmake_minimum_required(VERSION 2.8)

# Set the project name
project(hyperion-aml)

# find QT
if(ENABLE_QT5)
	find_package(Qt5Widgets REQUIRED)
else(ENABLE_QT5)
	find_package(Qt4 REQUIRED QtCore QtGui QtNetwork)
endif(ENABLE_QT5)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/protoserver
	${QT_INCLUDES}
	${PROTOBUF_INCLUDE_DIRS}
)

set(Hyperion_AML_QT_HEADERS
	AmlogicWrapper.h
)

set(Hyperion_AML_HEADERS
)

set(Hyperion_AML_SOURCES
	hyperion-aml.cpp
	AmlogicWrapper.cpp
)

if(ENABLE_QT5)
	QT5_WRAP_CPP(Hyperion_AML_HEADERS_MOC ${Hyperion_AML_QT_HEADERS})
else()
	QT4_WRAP_CPP(Hyperion_AML_HEADERS_MOC ${Hyperion_AML_QT_HEADERS})
endif()

add_executable(${PROJECT_NAME}
	${Hyperion_AML_HEADERS}
	${Hyperion_AML_SOURCES}
	${Hyperion_AML_HEADERS_MOC}
)

target_link_libraries(${PROJECT_NAME}
	getoptPlusPlus
	blackborder
	hyperion-utils
	protoserver
	amlogic-grabber
	pthread
)

if(ENABLE_QT5)
	qt5_use_modules(${PROJECT_NAME} Widgets Core Gui Network)
else()
	qt4_use_modules(${PROJECT_NAME} Core Gui Network )
endif()

install ( TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT ambilight )
