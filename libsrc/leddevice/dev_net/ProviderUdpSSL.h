#ifndef PROVIDERUDPSSL_H
#define PROVIDERUDPSSL_H

#include <leddevice/LedDevice.h>
#include <utils/Logger.h>

// Qt includes
#include <QMutex>
#include <QMutexLocker>
#include <QHostInfo>
#include <QThread>

//----------- mbedtls
#if defined(USE_MBEDTLS4)
#include <mbedtls/build_info.h>
#include <psa/crypto.h>
#elif defined(USE_MBEDTLS3)
#include <mbedtls/build_info.h>
#else
#if defined(__has_include)
#if __has_include(<mbedtls/build_info.h>)
#include <mbedtls/build_info.h>
#if defined(MBEDTLS_VERSION_MAJOR) && (MBEDTLS_VERSION_MAJOR >= 4)
#include <psa/crypto.h>
#ifndef USE_MBEDTLS4
#define USE_MBEDTLS4
#endif
#elif defined(MBEDTLS_VERSION_MAJOR) && (MBEDTLS_VERSION_MAJOR == 3)
#ifndef USE_MBEDTLS3
#define USE_MBEDTLS3
#endif
#endif
#elif !defined(MBEDTLS_CONFIG_FILE)
#include <mbedtls/config.h>
#else
#include MBEDTLS_CONFIG_FILE
#endif
#elif !defined(MBEDTLS_CONFIG_FILE)
#include <mbedtls/config.h>
#else
#include MBEDTLS_CONFIG_FILE
#endif
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include <mbedtls/platform.h>
#endif

#include <string.h>
#include <cstring>
#include <chrono>

#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl_ciphersuites.h>
#if !defined(USE_MBEDTLS4)
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#endif
#include <mbedtls/timing.h>
#include <mbedtls/error.h>
#include <mbedtls/debug.h>

class ProviderUdpSSL : public LedDevice
{
	Q_OBJECT

public:
	///
	/// @brief Constructs an UDP SSL LED-device
	///
	explicit ProviderUdpSSL(const QJsonObject &deviceConfig);

	///
	/// @brief Destructor of the LED-device
	///
	~ProviderUdpSSL() override;

	///
	QString      _hostName;
	int          _port;

protected:

	///
	/// @brief Initialise the UDP-SSL device's configuration and network address details
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Opens the output device.
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	int open() override;

	///
	/// @brief Closes the output device.
	///
	/// @return Zero on success (i.e. device is closed), else negative
	///
	int close() override;

	///
	/// @brief Initialise device's network details
	///
	/// @return True, if success
	///
	bool initNetwork();

	///
	/// @brief Start astreaming connection
	///
	/// @return True, if success
	///
	bool startConnection();

	///
	/// @brief Stop the streaming connection
	///
	void stopConnection();

	///
	/// Writes the given bytes/bits to the UDP-device and sleeps the latch time to ensure that the
	/// values are latched.
	///
	/// @param[in] data The data
	///
	void writeBytes(QByteArray data, bool flush = false);

	///
	/// Writes the given bytes/bits to the UDP-device and sleeps the latch time to ensure that the
	/// values are latched.
	///
	/// @param[in] size The length of the data
	/// @param[in] data The data
	///
	void writeBytes(unsigned int size, const uint8_t* data, bool flush = false);

	///
	/// get ciphersuites list from mbedtls_ssl_list_ciphersuites
	///
	/// @return const int * array
	///
	virtual const int * getCiphersuites() const;

	void setPSKidentity(const QString& pskIdentity);

private:

	bool initConnection();

#if !defined(USE_MBEDTLS4)
	bool seedingRNG();
#endif
	bool setupStructure();

	bool setupPSK();
	bool startSSLHandshake();

	QString errorMsg(int ret) const;
	void closeSSLNotify();
	void freeSSLConnection();

	mbedtls_net_context          client_fd;
#if !defined(USE_MBEDTLS4)
	mbedtls_entropy_context      entropy;
#endif
	mbedtls_ssl_context          ssl;
	mbedtls_ssl_config           conf;
	mbedtls_x509_crt             cacert;
#if !defined(USE_MBEDTLS4)
	mbedtls_ctr_drbg_context     ctr_drbg;
#endif
	mbedtls_timing_delay_context timer;

	QString      _transport_type;
	QString      _custom;
	int          _ssl_port;
	QString      _server_name;
	QString      _psk;
	QString      _psk_identity;

	int          _handshake_attempts;
	uint32_t     _handshake_timeout_min;
	uint32_t     _handshake_timeout_max;

	bool         _streamReady;
	bool         _streamPaused;
};

#endif // PROVIDERUDPSSL_H
