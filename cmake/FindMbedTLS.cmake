find_path(MBEDTLS_INCLUDE_DIR mbedtls/ssl.h)

find_library(MBEDTLS_SSL_LIBRARY mbedtls)
find_library(MBEDTLS_CRYPTO_LIBRARY mbedcrypto)
find_library(MBEDTLS_X509_LIBRARY mbedx509)

set(MBEDTLS_LIBRARIES ${MBEDTLS_SSL_LIBRARY} ${MBEDTLS_CRYPTO_LIBRARY} ${MBEDTLS_X509_LIBRARY})

if (MBEDTLS_INCLUDE_DIR AND EXISTS "${MBEDTLS_INCLUDE_DIR}/mbedtls/version.h")
    file(STRINGS "${MBEDTLS_INCLUDE_DIR}/mbedtls/version.h" _mbedtls_version_str REGEX "^#[\t ]*define[\t ]+MBEDTLS_VERSION_STRING[\t ]+\"[0-9]+.[0-9]+.[0-9]+\"")
    string(REGEX REPLACE "^.*MBEDTLS_VERSION_STRING.*([0-9]+.[0-9]+.[0-9]+).*" "\\1" MBEDTLS_VERSION "${_mbedtls_version_str}")
endif ()

include(FindPackageHandleStandardArgs)
if (MBEDTLS_VERSION)
    find_package_handle_standard_args(MbedTLS
        REQUIRED_VARS
            MBEDTLS_INCLUDE_DIR
            MBEDTLS_LIBRARIES
        VERSION_VAR
            MBEDTLS_VERSION
        FAIL_MESSAGE
            "Could NOT find mbedTLS, try to set the path to mbedTLS root folder
            in the system variable MBEDTLS_ROOT_DIR"
    )
else (MBEDTLS_VERSION)
    find_package_handle_standard_args(MbedTLS
        "Could NOT find mbedTLS, try to set the path to mbedLS root folder in
        the system variable MBEDTLS_ROOT_DIR"
        MBEDTLS_INCLUDE_DIR
        MBEDTLS_LIBRARIES)
endif (MBEDTLS_VERSION)

mark_as_advanced(MBEDTLS_INCLUDE_DIR MBEDTLS_LIBRARIES)
