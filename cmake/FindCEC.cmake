# - Try to find CEC
# Once done this will define
#
# CEC_FOUND - system has libcec
# CEC_INCLUDE_DIRS - the libcec include directory
# CEC_LIBRARIES - The libcec libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (CEC libcec>=3.0.0)
else()
  find_path(CEC_INCLUDE_DIRS libcec/cec.h)
  find_library(CEC_LIBRARIES cec)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CEC DEFAULT_MSG CEC_INCLUDE_DIRS CEC_LIBRARIES)

list(APPEND CEC_DEFINITIONS -DHAVE_LIBCEC=1)
mark_as_advanced(CEC_INCLUDE_DIRS CEC_LIBRARIES CEC_DEFINITIONS)
