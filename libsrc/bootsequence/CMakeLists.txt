
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/bootsequence)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/bootsequence)

# Group the headers that go through the MOC compiler
SET(BootsequenceHEADERS
		${CURRENT_HEADER_DIR}/BootSequence.h
		${CURRENT_HEADER_DIR}/BootSequenceFactory.h
		${CURRENT_SOURCE_DIR}/EffectBootSequence.h
)

SET(BootsequenceSOURCES
		${CURRENT_SOURCE_DIR}/BootSequenceFactory.cpp
		${CURRENT_SOURCE_DIR}/EffectBootSequence.cpp
)

add_library(bootsequence
		${BootsequenceHEADERS}
		${BootsequenceSOURCES}
)

target_link_libraries(bootsequence
		hyperion
		${QT_LIBRARIES})
