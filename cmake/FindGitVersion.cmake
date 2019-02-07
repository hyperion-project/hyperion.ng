
execute_process( COMMAND git log -1 --format=%cn-%t/%h-%ct WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE BUILD_ID ERROR_QUIET )
execute_process( COMMAND sh -c "git branch | grep '^*' | sed 's;^*;;g' " WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE VERSION_ID ERROR_QUIET )
execute_process( COMMAND sh -c "git remote --verbose | grep origin | grep fetch | cut -f2 | cut -d' ' -f1" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE GIT_REMOTE_PATH ERROR_QUIET )

STRING ( STRIP "${BUILD_ID}" BUILD_ID )
STRING ( STRIP "${VERSION_ID}" VERSION_ID )
STRING ( STRIP "${GIT_REMOTE_PATH}" GIT_REMOTE_PATH )
SET ( HYPERION_BUILD_ID "${VERSION_ID} (${BUILD_ID}) Git Remote: ${GIT_REMOTE_PATH}" )
message ( STATUS "Current Version: ${HYPERION_BUILD_ID}" )
