include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
ExternalProject_Add(
    webc
    PREFIX "webc"
    URL https://github.com/brindosch/hyperion-remote/releases/download/v${WEBCONFIG_VERSION}/Hyperion-Remote-${WEBCONFIG_VERSION}-EMBED.zip
    # TODO ADD HASH -> URL_HASH SHA512=59e7b43db838dbe6f02fda5e844d18e190d32d2ca1a83dc9f6b1aaed43e0340fc1e8ecabed6fffdac9f85bd34e7e93b5d8a17283d59ea3c14977a0372785d2bd
	SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets/webconfig/next"
	CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
