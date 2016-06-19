
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/amlogic)

# Group the headers that go through the MOC compiler
SET(AmlogicQT_HEADERS ${CURRENT_HEADER_DIR}/AmlogicWrapper.h)

SET(AmlogicHEADERS
	${CURRENT_HEADER_DIR}/AmlogicGrabber.h
)

SET(AmlogicSOURCES
	${CURRENT_SOURCE_DIR}/AmlogicWrapper.cpp
	${CURRENT_SOURCE_DIR}/AmlogicGrabber.cpp
)

QT5_WRAP_CPP(AmlogicHEADERS_MOC ${AmlogicQT_HEADERS})

add_library(amlogic-grabber
	${AmlogicHEADERS}
	${AmlogicQT_HEADERS}
	${AmlogicHEADERS_MOC}
	${AmlogicSOURCES}
)

target_link_libraries(amlogic-grabber
	hyperion
	${QT_LIBRARIES})
