
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/bootsequence)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/bootsequence)

# Group the headers that go through the MOC compiler
SET(BootsequenceQT_HEADERS
		${CURRENT_HEADER_DIR}/RainbowBootSequence.h
)

SET(BootsequenceHEADERS
)

SET(BootsequenceSOURCES
		${CURRENT_SOURCE_DIR}/RainbowBootSequence.cpp
)

QT4_WRAP_CPP(BootsequenceHEADERS_MOC ${BootsequenceQT_HEADERS})

add_library(bootsequence
		${BootsequenceHEADERS}
		${BootsequenceQT_HEADERS}
		${BootsequenceHEADERS_MOC}
		${BootsequenceSOURCES}
)

target_link_libraries(bootsequence
		hyperion
		${QT_LIBRARIES})
