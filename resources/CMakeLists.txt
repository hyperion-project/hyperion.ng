# Global resource file to embed static data into code, available as static lib 'resources'. File names needs to be unique in the /resources directory and below.
# All files are available with their file name by calling ":/FILENAME". Probably you need to call Q_INIT_RESOURCE("resources") once
#
# Define the current source locations
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)

# catch all files
file(GLOB Hyperion_RESFILES "${CURRENT_SOURCE_DIR}/icons/*" "${CURRENT_SOURCE_DIR}/ssl/*")

# fill resources.qrc with RESFILES
foreach(f ${Hyperion_RESFILES})
	get_filename_component(fname ${f} NAME)
	set(HYPERION_RES "${HYPERION_RES}\n\t\t<file alias=\"${fname}\">${f}</file>")
endforeach()

# prep file
configure_file(${CURRENT_SOURCE_DIR}/resources.qrc.in ${CMAKE_BINARY_DIR}/resources.qrc)
set(Hyperion_RES ${CMAKE_BINARY_DIR}/resources.qrc)

add_library(resources
	${Hyperion_RES}
)
