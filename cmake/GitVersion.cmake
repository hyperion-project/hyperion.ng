include_guard(GLOBAL)

execute_process(
    COMMAND git config --global --add safe.directory "*"
    ERROR_QUIET
)

execute_process(
	COMMAND git log -1 --format=%cn-%t/%h-%ct
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	OUTPUT_VARIABLE BUILD_ID
	ERROR_QUIET
)

execute_process(
	COMMAND git symbolic-ref --short HEAD
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	OUTPUT_VARIABLE VERSION_ID
	ERROR_QUIET
)

execute_process(
	COMMAND git config --get remote.origin.url
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	OUTPUT_VARIABLE GIT_REMOTE_PATH
	ERROR_QUIET
)

string(STRIP "${BUILD_ID}" HYPERION_BUILD_ID)
string(STRIP "${VERSION_ID}" HYPERION_VERSION_ID)
string(STRIP "${GIT_REMOTE_PATH}" HYPERION_GIT_REMOTE)
