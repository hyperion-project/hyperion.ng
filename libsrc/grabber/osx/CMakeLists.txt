# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/osx)

# Group the headers that go through the MOC compiler
SET(OsxGrabberQT_HEADERS
		${CURRENT_HEADER_DIR}/OsxWrapper.h
)

SET(OsxGrabberHEADERS
		${CURRENT_SOURCE_DIR}/OsxFrameGrabber.h
)

SET(OsxGrabberSOURCES
		${CURRENT_SOURCE_DIR}/OsxWrapper.cpp
		${CURRENT_SOURCE_DIR}/OsxFrameGrabber.cpp
)

if(ENABLE_QT5)
QT5_WRAP_CPP(OsxGrabberHEADERS_MOC ${OsxGrabberQT_HEADERS})
else(ENABLE_QT5)
QT4_WRAP_CPP(OsxGrabberHEADERS_MOC ${OsxGrabberQT_HEADERS})
endif(ENABLE_QT5)

add_library(osx-grabber
		${OsxGrabberHEADERS}
		${OsxGrabberQT_HEADERS}
		${OsxGrabberHEADERS_MOC}
		${OsxGrabberSOURCES}
)

target_link_libraries(osx-grabber
		hyperion
		${QT_LIBRARIES})
