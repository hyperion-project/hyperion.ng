# - Try to find libtinkerforge-1.0
# Once done this will define
#
#  LIBTINKERFORGE_1_FOUND - system has libtinkerforge
#  LIBTINKERFORGE_1_INCLUDE_DIRS - the libtinkerforge include directory
#  LIBTINKERFORGE_1_LIBRARIES - Link these to use libtinkerforge
#  LIBTINKERFORGE_1_DEFINITIONS - Compiler switches required for using libtinkerforge
#
#  Adapted from cmake-modules Google Code project
#
#  Copyright (c) 2006 Andreas Schneider <mail@cynapses.org>
#
#  (Changes for libtinkerforge) Copyright (c) 2014 Björn Bilger <bjoern.bilger@googlemail.com>
#
# Redistribution and use is allowed according to the terms of the New BSD license.
#
# CMake-Modules Project New BSD License
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of the CMake-Modules Project nor the names of its
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#


if (LIBTINKERFORGE_1_LIBRARIES AND LIBTINKERFORGE_1_INCLUDE_DIRS)
  # in cache already
  set(LIBTINKERFORGE_FOUND TRUE)
else (LIBTINKERFORGE_1_LIBRARIES AND LIBTINKERFORGE_1_INCLUDE_DIRS)
  find_path(LIBTINKERFORGE_1_INCLUDE_DIR
	NAMES
	tinkerforge/tinkerforge.h
	PATHS
	  /usr/include
	  /usr/local/include
	  /opt/local/include
	  /sw/include
	PATH_SUFFIXES
	  tinkerforge-1.0
  )

  find_library(LIBTINKERFORGE_1_LIBRARY
	NAMES
	  tinkerforge-1.0
	PATHS
	  /usr/lib
	  /usr/local/lib
	  /opt/local/lib
	  /sw/lib
  )

  set(LIBTINKERFORGE_1_INCLUDE_DIRS
	${LIBTINKERFORGE_1_INCLUDE_DIR}
  )
  set(LIBTINKERFORGE_1_LIBRARIES
	${LIBTINKERFORGE_1_LIBRARY}
)

  if (LIBTINKERFORGE_1_INCLUDE_DIRS AND LIBTINKERFORGE_1_LIBRARIES)
	 set(LIBTINKERFORGE_1_FOUND TRUE)
  endif (LIBTINKERFORGE_1_INCLUDE_DIRS AND LIBTINKERFORGE_1_LIBRARIES)

  if (LIBTINKERFORGE_1_FOUND)
	if (NOT libtinkerforge_1_FIND_QUIETLY)
	  message(STATUS "Found libtinkerforge-1.0:")
	  message(STATUS " - Includes: ${LIBTINKERFORGE_1_INCLUDE_DIRS}")
	  message(STATUS " - Libraries: ${LIBTINKERFORGE_1_LIBRARIES}")
	endif (NOT libtinkerforge_1_FIND_QUIETLY)
  else (LIBTINKERFORGE_1_FOUND)
	  message(FATAL_ERROR "Could not find libtinkerforge")
  endif (LIBTINKERFORGE_1_FOUND)

  # show the LIBTINKERFORGE_1_INCLUDE_DIRS and LIBTINKERFORGE_1_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBTINKERFORGE_1_INCLUDE_DIRS LIBTINKERFORGE_1_LIBRARIES)

endif (LIBTINKERFORGE_1_LIBRARIES AND LIBTINKERFORGE_1_INCLUDE_DIRS)
