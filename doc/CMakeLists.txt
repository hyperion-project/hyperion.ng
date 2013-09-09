
# Find doxygen
find_package(Doxygen QUIET)

# This processes our hyperion-cmake.doxyfile and subsitutes variables to generate a final hyperion.doxyfile
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/hyperion.in.doxygen ${CMAKE_CURRENT_BINARY_DIR}/hyperion.doxygen)

# This processes the shell script that is used to build the documentation and check the result
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/hyperion-build-doc.in.sh ${CMAKE_CURRENT_BINARY_DIR}/hyperion-build-doc.sh)

# Define all static (i.e. not generated) documentation files
set(StaticDocumentationFiles hyperion-header.html hyperion-footer.html hyperion-stylesheet.css)

# Loop over all static documentation files
foreach(StaticDocumentationFile ${StaticDocumentationFiles})
	# Copy the file to the bindary documentation directory
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${StaticDocumentationFile} ${CMAKE_CURRENT_BINARY_DIR}/html/${StaticDocumentationFile} COPYONLY)
endforeach()

if(DOXYGEN_FOUND)
	option(BuildDocumentationSearchEngine "Enable doxygen's search engine (requires that documentation to be installed on a php enabled web server)" OFF)
	if(BuildDocumentationSearchEngine)
		set(DOXYGEN_SEARCHENGINE YES)
	else(BuildDocumentationSearchEngine)
		set(DOXYGEN_SEARCHENGINE NO)
	endif(BuildDocumentationSearchEngine)

	#Create a custom target to build documentation. It runs doxygen aginast the generated hyperion.doxyfile and checks its return value
	add_custom_target(doc sh ${CMAKE_CURRENT_BINARY_DIR}/hyperion-build-doc.sh)
else(DOXYGEN_FOUND)
	message(WARNING "Doxygen not found, unable to generate documenation!")
endif(DOXYGEN_FOUND)
