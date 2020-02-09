include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
ExternalProject_Add(
    webc
    PREFIX "webc"
    URL https://github.com/brindosch/hyperion-remote/releases/download/v${WEBCONFIG_VERSION}/Hyperion-Remote-${WEBCONFIG_VERSION}-EMBED.zip
    URL_HASH SHA256=${WEBCONFIG_FILE_SHA256}
	SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets/webconfig/next"
	CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
