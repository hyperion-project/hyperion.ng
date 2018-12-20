option(BUILD_HYPERION_DOC "Build hyperion documentation" OFF)

# Find doxygen and check if Doxygen is installed
find_package(Doxygen QUIET)

if (BUILD_HYPERION_DOC)
	if (DOXYGEN_FOUND)

		# set input and output files
		set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/hyperion.in.doxygen)
		set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/hyperion.doxygen)

		# request to configure the file
		configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
		message(STATUS "Doxygen build started")

		# Define all static (i.e. not generated) documentation files
		set(StaticDocumentationFileshyperion-footer.html)

		# Loop over all static documentation files
		foreach(StaticDocumentationFile ${StaticDocumentationFiles})
			# Copy the file to the bindary documentation directory
			configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${StaticDocumentationFile} ${CMAKE_CURRENT_BINARY_DIR}/html/${StaticDocumentationFile} COPYONLY)
		endforeach()

		add_custom_target( doc_doxygen ALL
			COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Generating API documentation with Doxygen"
			VERBATIM )
	else(DOXYGEN_FOUND)
		message(WARNING "Doxygen not found, unable to generate documenation!")
	endif(DOXYGEN_FOUND)
endif()
