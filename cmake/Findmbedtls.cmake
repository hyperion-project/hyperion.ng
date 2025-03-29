# mbedtls
# mbedx509
# mbedcrypto

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(PC_MbedTLS QUIET mbedtls mbedcrypto mbedx509)
endif()

find_path(mbedtls_INCLUDE_DIR
    NAMES
		mbedtls/ssl.h
	HINTS
		${PC_MbedTLS_INCLUDE_DIRS}
	PATHS
		/usr/include
		/usr/local/include
)

find_library(mbedtls_LIBRARY
	NAMES
		libmbedtls
		mbedtls
	HINTS
		${PC_MbedTLS_LIBRARY_DIRS}
	PATHS
		/usr/lib
		/usr/local/lib
)

find_library(mbedx509_LIBRARY
	NAMES
		libmbedx509
		mbedx509
	HINTS
		${PC_MbedTLS_LIBRARY_DIRS}
	PATHS
		/usr/lib
		/usr/local/lib
)

find_library(mbedcrypto_LIBRARY
	NAMES
		libmbedcrypto
		mbedcrypto
	HINTS
		${PC_MbedTLS_LIBRARY_DIRS}
	PATHS
		/usr/lib
		/usr/local/lib
)

include(CheckSymbolExists)
if(mbedtls_LIBRARY AND NOT mbedx509_LIBRARY AND NOT mbedcrypto_LIBRARY)
	set(CMAKE_REQUIRED_INCLUDES "${mbedtls_INCLUDE_DIR}")
	set(CMAKE_REQUIRED_LIBRARIES "${mbedtls_LIBRARY}")

	check_symbol_exists(mbedtls_x509_crt_init "mbedtls/x590_crt.h" MBEDTLS_INCLUDES_X509)
	check_symbol_exists(mbedtls_sha256_init "mbedtls/sha256.h" MBEDTLS_INCLUDES_CRYPTO)

	unset(CMAKE_REQUIRED_INCLUDES)
	unset(CMAKE_REQUIRED_LIBRARIES)
endif()

include(FindPackageHandleStandardArgs)
if(MBEDTLS_INCLUDES_X509 AND MBEDTLS_INCLUDES_CRYPTO)
	find_package_handle_standard_args(MbedTLS
		FOUND_VAR MbedTLS_FOUND
		REQUIRED_VARS mbedtls_INCLUDE_DIR mbedtls_LIBRARY
	)

	mark_as_advanced(mbedtls_INCLUDE_DIR mbedtls_LIBRARY)
	list(APPEND COMPONENTS tls)
else()
	find_package_handle_standard_args(MbedTLS
		FOUND_VAR MbedTLS_FOUND
		REQUIRED_VARS mbedtls_INCLUDE_DIR mbedtls_LIBRARY mbedx509_LIBRARY mbedcrypto_LIBRARY
	)

  	mark_as_advanced(mbedtls_INCLUDE_DIR mbedtls_LIBRARY mbedx509_LIBRARY mbedcrypto_LIBRARY)
	list(APPEND COMPONENTS tls x509 crypto)
endif()

if(MbedTLS_FOUND)
	foreach(COMP IN LISTS COMPONENTS)
		if(NOT TARGET MbedTLS::mbed${COMP})
			if(IS_ABSOLUTE "${mbed${COMP}_LIBRARY}")
				add_library(MbedTLS::mbed${COMP} UNKNOWN IMPORTED)
				set_target_properties(MbedTLS::mbed${COMP} PROPERTIES
					IMPORTED_LOCATION "${mbed${COMP}_LIBRARY}"
				)
			else()
				add_library(MbedTLS::mbed${COMP} INTERFACE IMPORTED)
				set_target_properties(MbedTLS::mbed${COMP} PROPERTIES
					IMPORTED_LIBNAME "${mbed${COMP}_LIBRARY}"
				)
			endif()
		endif()
	endforeach()

	set_target_properties(MbedTLS::mbedtls PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${mbedtls_INCLUDE_DIR}"
	)

	if(NOT MBEDTLS_INCLUDES_X509 AND NOT MBEDTLS_INCLUDES_CRYPTO)
		set_property(TARGET MbedTLS::mbedtls PROPERTY
			INTERFACE_LINK_LIBRARIES MbedTLS::mbedcrypto MbedTLS::mbedx509
		)
	endif()
endif()
