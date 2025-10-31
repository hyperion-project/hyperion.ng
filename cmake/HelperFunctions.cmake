include_guard(GLOBAL)

# Extract Major.Minor.Patch.Pre version from string
function(SetVersionNumber PREFIX VERSION_STRING)
	string(REGEX MATCHALL "[0-9]+|-([A-Za-z0-9_.]+)" VERSION_PARTS ${VERSION_STRING})
	list(LENGTH VERSION_PARTS VERSION_LEN)

	if(${VERSION_LEN} GREATER 0)
		list(GET VERSION_PARTS 0 VERSION_MAJOR)
	endif()

	if(${VERSION_LEN} GREATER 1)
		list(GET VERSION_PARTS 1 VERSION_MINOR)
	endif()

	if(${VERSION_LEN} GREATER 2)
		list(GET VERSION_PARTS 2 VERSION_PATCH)
	endif()

	if(${VERSION_LEN} GREATER 3)
		list(GET VERSION_PARTS 3 VERSION_PRE)
	endif()

	set(${PREFIX}_VERSION_MAJOR ${VERSION_MAJOR} PARENT_SCOPE)
	set(${PREFIX}_VERSION_MINOR ${VERSION_MINOR} PARENT_SCOPE)
	set(${PREFIX}_VERSION_PATCH ${VERSION_PATCH} PARENT_SCOPE)
	set(${PREFIX}_VERSION_PRE   ${VERSION_PRE}   PARENT_SCOPE)
endfunction()

# Determine the current Platform and put the result into the OUTPUT var
function(DeterminePlatform OUTPUT)
	set(_output "")
	if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
		set(_output "osx")
	elseif (WIN32)
		set(_output "windows")
	elseif ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "x86")
		set(_output "x11")
	elseif ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "arm" OR "${CMAKE_SYSTEM_PROCESSOR}" MATCHES "aarch64")
		set(_output "rpi")
		file(READ /proc/cpuinfo SYSTEM_CPUINFO)
		string(TOLOWER "${SYSTEM_CPUINFO}" SYSTEM_CPUINFO)
		if("${SYSTEM_CPUINFO}" MATCHES "amlogic" AND ${CMAKE_SIZEOF_VOID_P} EQUAL 4)
			set(_output "amlogic")
		elseif (("${SYSTEM_CPUINFO}" MATCHES "amlogic" OR "${SYSTEM_CPUINFO}" MATCHES "odroid-c2" OR "${SYSTEM_CPUINFO}" MATCHES "vero4k") AND ${CMAKE_SIZEOF_VOID_P} EQUAL 8)
			set(_output "amlogic64")
		endif()
	endif()

	set(${OUTPUT} "${_output}" PARENT_SCOPE)
endfunction()

# Put the TARGET's build interface include directory into the OUTPUT var
include(CMakeParseArguments)
function(get_build_interface_include_directory)
	cmake_parse_arguments(arg "" "TARGET;OUTPUT" "" ${ARGN})

	if(NOT arg_TARGET)
		message(SEND_ERROR "TARGET is a required argument")
		return()
	endif()

	if(NOT arg_OUTPUT)
		message(SEND_ERROR "OUTPUT is a required argument")
		return()
	endif()

	if(NOT TARGET ${arg_TARGET})
		set(${arg_OUTPUT} "${arg_TARGET}-NOTFOUND" PARENT_SCOPE)
		return()
	endif()

	set(_output "")
	get_target_property(_output ${arg_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
	foreach(include ${_output})
		if(include MATCHES "\\$<BUILD_INTERFACE:([^,>]+)>")
			set(_output "${CMAKE_MATCH_1}")
		endif()
	endforeach()

	set(${arg_OUTPUT} "${_output}" PARENT_SCOPE)
endfunction()
