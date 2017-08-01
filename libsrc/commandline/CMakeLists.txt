# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/commandline)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/commandline)

set(Parser_HEADERS
        ${CURRENT_HEADER_DIR}/ColorOption.h
        ${CURRENT_HEADER_DIR}/ColorsOption.h
        ${CURRENT_HEADER_DIR}/DoubleOption.h
        ${CURRENT_HEADER_DIR}/ImageOption.h
        ${CURRENT_HEADER_DIR}/IntOption.h
        ${CURRENT_HEADER_DIR}/Option.h
        ${CURRENT_HEADER_DIR}/Parser.h
        ${CURRENT_HEADER_DIR}/RegularExpressionOption.h
        ${CURRENT_HEADER_DIR}/SwitchOption.h
        ${CURRENT_HEADER_DIR}/ValidatorOption.h
        ${CURRENT_HEADER_DIR}/BooleanOption.h
)

set(Parser_SOURCES
        ${CURRENT_SOURCE_DIR}/ColorOption.cpp
        ${CURRENT_SOURCE_DIR}/ColorsOption.cpp
        ${CURRENT_SOURCE_DIR}/DoubleOption.cpp
        ${CURRENT_SOURCE_DIR}/ImageOption.cpp
        ${CURRENT_SOURCE_DIR}/IntOption.cpp
        ${CURRENT_SOURCE_DIR}/Option.cpp
        ${CURRENT_SOURCE_DIR}/Parser.cpp
        ${CURRENT_SOURCE_DIR}/RegularExpressionOption.cpp
        ${CURRENT_SOURCE_DIR}/SwitchOption.cpp
        ${CURRENT_SOURCE_DIR}/ValidatorOption.cpp
        ${CURRENT_SOURCE_DIR}/BooleanOption.cpp
)

add_library(commandline
    ${Parser_HEADERS}
    ${Parser_SOURCES}
)

target_link_libraries(commandline
    hyperion
    Qt5::Gui
)
