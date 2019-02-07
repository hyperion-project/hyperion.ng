# - Find package for .rpm building
# Find the .rpm building executable and extract the version number
#
# OUTPUT Variables
#
#   RPM_BUILDER_FOUND
#       True if the rpm package was found
#   RPM_BUILDER_EXECUTABLE
#       The rpm executable location
#   RPM_BUILDER_VERSION
#       A string denoting the version of rpm that has been found

find_program ( RPM_BUILDER_EXECUTABLE rpm )

if ( RPM_BUILDER_EXECUTABLE )
  SET( RPM_BUILDER_FOUND TRUE )
  execute_process ( COMMAND ${RPM_BUILDER_EXECUTABLE} --version OUTPUT_VARIABLE RPM_VERSION_RAW ERROR_QUIET )
  if (RPM_VERSION_RAW)
    string ( REGEX REPLACE "^RPM-Version ([0-9]+.[0-9]+.[0-9]+),.*" "\\1" RPM_BUILDER_VERSION ${RPM_VERSION_RAW})
  else ()
    set ( RPM_BUILDER_VERSION "unknown" )
  endif()
endif ()
