# Downloads the pre-built dependencies described in PreBuiltPresets.json and sets PRE_BUILT_DEPS_DIR
# to the extracted directory. If anything goes wrong, PRE_BUILT_DEPS_DIR stays empty (-> full build).
include_guard(GLOBAL)

if(CMAKE_VERSION VERSION_LESS "3.19")
	message(WARNING "Fetching pre-built dependencies requires CMake >= 3.19 (found ${CMAKE_VERSION}), building all dependencies instead")
	return()
endif()

# OS key (macos | windows | <distro>_<codename>) and architecture key of the pre-built dependencies
if(APPLE)
	set(_os "macos")
elseif(WIN32)
	set(_os "windows")
elseif(EXISTS "/etc/os-release")
	file(STRINGS "/etc/os-release" _id REGEX "^ID=")
	file(STRINGS "/etc/os-release" _codename REGEX "^VERSION_CODENAME=")
	string(REGEX REPLACE "^[A-Z_]+=|\"" "" _id "${_id}")
	string(REGEX REPLACE "^[A-Z_]+=|\"" "" _codename "${_codename}")
	# Raspbian is a Debian rebuild for armv6 and uses the Debian pre-built dependencies
	if(_id STREQUAL "raspbian")
		set(_id "debian")
	endif()
	set(_os "${_id}_${_codename}")
else()
	set(_os "unknown")
endif()

# Inside emulated containers the kernel reports the host architecture (e.g. armv7l/aarch64 for armv6),
# so an explicitly given CPACK_SYSTEM_PROCESSOR takes precedence
if(CPACK_SYSTEM_PROCESSOR)
	string(TOLOWER "${CPACK_SYSTEM_PROCESSOR}" _arch)
else()
	string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _arch)
endif()

string(REGEX REPLACE "^aarch64$" "arm64" _arch "${_arch}")
string(REGEX REPLACE "^(armv[67]).*" "\\1" _arch "${_arch}")
if(_arch MATCHES "^(x86_64|amd64)$")
	if(APPLE OR WIN32)
		set(_arch "x64")
	else()
		set(_arch "amd64")
	endif()
endif()

file(READ "${CMAKE_SOURCE_DIR}/PreBuiltSpecs.json" _deps)

string(TOLOWER "${CMAKE_BUILD_TYPE}" _type)
if(NOT _type)
	set(_type "release")
endif()

string(JSON _hash ERROR_VARIABLE _err GET "${_deps}" hashes ${_os} ${_arch} ${_type})
if(_err)
	message(WARNING "No pre-built dependencies for ${_os}-${_arch}-${_type}, building all dependencies instead")
	return()
endif()
string(JSON _qt      GET "${_deps}" hashes ${_os} ${_arch} qt_version)
string(JSON _version GET "${_deps}" version)
string(JSON _url     GET "${_deps}" baseUrl)

# The directory name contains version, Qt and build type -> a new release is fetched automatically
set(_name "${_os}-${_arch}-qt${_qt}-${_type}-${_version}")
set(_dir "${CMAKE_BINARY_DIR}/.deps/${_name}")

if(NOT EXISTS "${_dir}")
	set(_archive "${_dir}.tar.gz")

	message(STATUS "Downloading pre-built dependencies: ${_name}")
	file(DOWNLOAD "${_url}/${_version}/${_name}.tar.gz" "${_archive}"
		EXPECTED_HASH SHA256=${_hash}
		INACTIVITY_TIMEOUT 60
		STATUS _status
	)
	list(GET _status 0 _code)
	if(NOT _code EQUAL 0)
		list(GET _status 1 _msg)
		file(REMOVE "${_archive}")
		message(WARNING "Download of ${_name} failed (${_msg}), building all dependencies instead")
		return()
	endif()

	# Extract to a temporary directory first, so an aborted run never leaves a half-extracted folder behind
	file(REMOVE_RECURSE "${_dir}.tmp")
	file(MAKE_DIRECTORY "${_dir}.tmp")
	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf "${_archive}"
		WORKING_DIRECTORY "${_dir}.tmp"
		RESULT_VARIABLE _code
	)
	file(REMOVE "${_archive}")
	if(NOT _code EQUAL 0)
		file(REMOVE_RECURSE "${_dir}.tmp")
		message(WARNING "Extracting ${_name} failed, building all dependencies instead")
		return()
	endif()

	# flatc and protoc must be executable (not guaranteed by the archive)
	file(GLOB _flat_proto "${_dir}.tmp/bin/*")
	if(_flat_proto AND NOT WIN32)
		file(CHMOD ${_flat_proto} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
	endif()

	file(RENAME "${_dir}.tmp" "${_dir}")
endif()

set(PRE_BUILT_DEPS_DIR "${_dir}")
message(STATUS "Pre-built dependencies: ${PRE_BUILT_DEPS_DIR}")
