#pragma once

#include <leddevice/LedDevice.h>
#include <utils/Logger.h>

// Qt includes
#include <QMutex>
#include <QMutexLocker>
#include <QHostInfo>
#include <QThread>

//----------- mbedtls

#if !defined(MBEDTLS_CONFIG_FILE)
#include <mbedtls/config.h>
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include <mbedtls/platform.h>
#else
#include <stdio.h>
#include <stdlib.h>
#define mbedtls_time         time
#define mbedtls_time_t       time_t
#define mbedtls_printf       printf
#define mbedtls_fprintf      fprintf
#define mbedtls_snprintf     snprintf
#define mbedtls_calloc       calloc
#define mbedtls_free         free
#define mbedtls_exit         exit
#define MBEDTLS_EXIT_SUCCESS EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE EXIT_FAILURE
#endif

#include <string.h>
#include <cstring>

#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl_ciphersuites.h>
#include <mbedtls/entropy.h>
#include <mbedtls/timing.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/error.h>
#include <mbedtls/debug.h>

#define READ_TIMEOUT_MS 1000
#define MAX_RETRY       5

//----------- END mbedtls

const ushort MAX_PORT_SSL = 65535;

class ProviderUdpSSL : public LedDevice
{
	Q_OBJECT

public:
	///
	/// Constructs specific LedDevice
	///
	ProviderUdpSSL();

	///
	/// Destructor of the LedDevice; closes the output device if it is open
	///
	virtual ~ProviderUdpSSL() override;

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	virtual bool init(const QJsonObject &deviceConfig) override;

public slots:
	///
	/// Closes the output device.
	/// Includes switching-off the device and stopping refreshes
	///
	virtual void close() override;

protected:

	///
	/// Initialise device's network details
	///
	/// @return True if success
	///
	bool initNetwork();

	///
	/// Opens and configures the output device
	///
	/// @return Zero on succes else negative
	///
	int open() override;

	///
	/// Writes the given bytes/bits to the UDP-device and sleeps the latch time to ensure that the
	/// values are latched.
	///
	/// @param[in] size The length of the data
	/// @param[in] data The data
	///
	void writeBytes(const unsigned size, const uint8_t *data);

	///
	/// get ciphersuites list from mbedtls_ssl_list_ciphersuites
	///
	/// @return const int * array
	///
	virtual const int * getCiphersuites();

	void log(const QString &msg, const char* errorType = "debug");
	void log(const char* msg, const char* errorType = "debug");
	void configLog(const char* msg, const char* type, ...);

	/**
	 * Debug callback for mbed TLS
	 * Just prints on the USB serial port
	 */
	static void ProviderUdpSSLDebug(void *ctx, int level, const char *file, int line, const char *str)
	{
		const char *p, *basename;
		(void) ctx;
		/* Extract basename from file */
		for(p = basename = file; *p != '\0'; p++)
		{
			if(*p == '/' || *p == '\\')
			{
				basename = p + 1;
			}
		}
		mbedtls_printf("%s:%04d: |%d| %s", basename, line, level, str);
	}

	/**
	 * Certificate verification callback for mbed TLS
	 * Here we only use it to display information on each cert in the chain
	 */
	static int ProviderUdpSSLVerify(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags)
	{
		const uint32_t buf_size = 1024;
		char *buf = new char[buf_size];
		(void) data;

		mbedtls_printf("\nVerifying certificate at depth %d:\n", depth);
		mbedtls_x509_crt_info(buf, buf_size - 1, "  ", crt);
		mbedtls_printf("%s", buf);

		if (*flags == 0)
			mbedtls_printf("No verification issue for this certificate\n");
		else
		{
			mbedtls_x509_crt_verify_info(buf, buf_size, "  ! ", *flags);
			mbedtls_printf("%s\n", buf);
		}

		delete[] buf;
		return 0;
	}

	///
	/// closeSSLNotify and freeSSLConnection
	///
	void closeSSLConnection();

private:

	bool buildConnection();
	bool initConnection();
	bool seedingRNG();
	bool setupStructure();
	bool startUPDConnection();
	bool setupPSK();
	bool startSSLHandshake();
	void handleReturn(int ret);
	QString errorMsg(int ret);
	void closeSSLNotify();
	void freeSSLConnection();

	mbedtls_net_context          client_fd;
	mbedtls_entropy_context      entropy;
	mbedtls_ssl_context          ssl;
	mbedtls_ssl_config           conf;
	mbedtls_x509_crt             cacert;
	mbedtls_ctr_drbg_context     ctr_drbg;
	mbedtls_timing_delay_context timer;

	QMutex       _hueMutex;
	QString      _transport_type;
	QString      _custom;
	QHostAddress _address;
	QString      _defaultHost;
	int          _port;
	int          _ssl_port;
	QString      _server_name;
	QString      _psk;
	QString      _psk_identity;
	uint32_t     _read_timeout;
	uint32_t     _handshake_timeout_min;
	uint32_t     _handshake_timeout_max;
	unsigned int _handshake_attempts;
	int          _retry_left;
	bool         _stopConnection;
	bool         _debugStreamer;
	int          _debugLevel;
};
