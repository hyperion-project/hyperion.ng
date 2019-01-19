# - Find package for .deb building
# Find the .deb building executable and extract the version number
#
# OUTPUT Variables
#
#   DEB_BUILDER_FOUND
#       True if the deb builder package was found
#   DEB_BUILDER_EXECUTABLE
#       The deb builder executable location
#   DEB_BUILDER_VERSION
#       A string denoting the version of deb builder that has been found

find_program ( DEB_BUILDER_EXECUTABLE dpkg-deb )

if ( DEB_BUILDER_EXECUTABLE )
  SET( DEB_BUILDER_FOUND TRUE )
  execute_process ( COMMAND ${DEB_BUILDER_EXECUTABLE} --version OUTPUT_VARIABLE DEB_VERSION_RAW ERROR_QUIET )
  if (DEB_VERSION_RAW)
    string ( REGEX REPLACE "^RPM-Version ([0-9]+.[0-9]+.[0-9]+),.*" "\\1" DEB_BUILDER_VERSION ${DEB_VERSION_RAW})
  else ()
    set ( DEB_BUILDER_VERSION "unknown" )
  endif()
endif ()
