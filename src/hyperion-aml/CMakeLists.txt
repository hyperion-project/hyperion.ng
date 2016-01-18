# Configure minimum CMAKE version
cmake_minimum_required(VERSION 2.8)

# Set the project name
project(hyperion-aml)

# find Qt4
find_package(Qt4 REQUIRED QtCore QtGui QtNetwork)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/protoserver
	${QT_INCLUDES}
	${PROTOBUF_INCLUDE_DIRS}
)

set(Hyperion_AML_QT_HEADERS
)

set(Hyperion_AML_HEADERS
)

set(Hyperion_AML_SOURCES
	hyperion-aml.cpp
)

if(ENABLE_QT5)
QT5_WRAP_CPP(Hyperion_AML_HEADERS_MOC ${Hyperion_AML_QT_HEADERS})
else(ENABLE_QT5)
QT4_WRAP_CPP(Hyperion_AML_HEADERS_MOC ${Hyperion_AML_QT_HEADERS})
endif(ENABLE_QT5)

add_executable(hyperion-amlogic
	${Hyperion_AML_HEADERS}
	${Hyperion_AML_SOURCES}
	${Hyperion_AML_HEADERS_MOC}
)

target_link_libraries(hyperion-amlogic
	getoptPlusPlus
	blackborder
	hyperion-utils
	protoserver
	amlogic-grabber
	pthread
)

qt4_use_modules(hyperion-amlogic
	Core
	Gui
	Network)
