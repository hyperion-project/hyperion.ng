
// STL includes
#include <cstdio>
#include <exception>
#include <algorithm>

// Linux includes
#include <fcntl.h>
#ifndef _WIN32
	#include <sys/ioctl.h>
#endif

// Local Hyperion includes
#include "ProviderUdpSSL.h"
#include <utils/NetUtils.h>

#include "mbedtls/version.h"

// Constants
namespace {

const int DEFAULT_SSLPORT = 2100;

const char DEFAULT_TRANSPORT_TYPE[] = "DTLS";
const char DEFAULT_SEED_CUSTOM[] = "dtls_client";

const int DEFAULT_HANDSHAKE_ATTEMPTS = 5;
const int DEFAULT_HANDSHAKE_TIMEOUT_MIN = 300;
const int DEFAULT_HANDSHAKE_TIMEOUT_MAX = 1000;
}


ProviderUdpSSL::ProviderUdpSSL(const QJsonObject &deviceConfig)
	: LedDevice(deviceConfig)
	, _port(-1)
	, client_fd()
	, entropy()
	, ssl()
	, conf()
	, cacert()
	, ctr_drbg()
	, timer()
	, _transport_type(DEFAULT_TRANSPORT_TYPE)
	, _custom(DEFAULT_SEED_CUSTOM)
	, _ssl_port(1)
	, _server_name()
	, _psk()
	, _psk_identity()
	, _handshake_attempts(DEFAULT_HANDSHAKE_ATTEMPTS)
	, _handshake_timeout_min(DEFAULT_HANDSHAKE_TIMEOUT_MIN)
	, _handshake_timeout_max(DEFAULT_HANDSHAKE_TIMEOUT_MAX)
	, _streamReady(false)
	, _streamPaused(false)

{
	bool error = false;

	try
	{
		mbedtls_ctr_drbg_init(&ctr_drbg);
		error = !seedingRNG();
	}
	catch (...)
	{
		error = true;
	}

	if (error)
	{
		Error(_log, "Failed to initialize mbedtls seed");
	}
}

ProviderUdpSSL::~ProviderUdpSSL()
{
	stopConnection();

	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);
}

bool ProviderUdpSSL::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = false;

	// Initialise sub-class
	if ( LedDevice::init(deviceConfig) )
	{
		//PSK Pre Shared Key
		_psk           = deviceConfig["psk"].toString();
		_psk_identity  = deviceConfig["psk_identity"].toString();
		_ssl_port      = deviceConfig["sslport"].toInt(DEFAULT_SSLPORT);
		_server_name   = deviceConfig["servername"].toString();

		if( deviceConfig.contains("transport_type") ) { _transport_type        = deviceConfig["transport_type"].toString(DEFAULT_TRANSPORT_TYPE); }
		if( deviceConfig.contains("seed_custom") )    { _custom                = deviceConfig["seed_custom"].toString(DEFAULT_SEED_CUSTOM); }
		if( deviceConfig.contains("hs_attempts") )    { _handshake_attempts    = deviceConfig["hs_attempts"].toInt(DEFAULT_HANDSHAKE_ATTEMPTS); }
		if (deviceConfig.contains("hs_timeout_min"))  { _handshake_timeout_min = static_cast<uint32_t>(deviceConfig["hs_timeout_min"].toInt(DEFAULT_HANDSHAKE_TIMEOUT_MIN)); }
		if (deviceConfig.contains("hs_timeout_max"))  { _handshake_timeout_max = static_cast<uint32_t>(deviceConfig["hs_timeout_max"].toInt(DEFAULT_HANDSHAKE_TIMEOUT_MAX)); }

		if (!NetUtils::isValidPort(_log,_ssl_port,_server_name))
		{
			QString errortext = QString ("Invalid SSL port [%1]!").arg(_ssl_port);
			this->setInError( errortext );
			isInitOK = false;
		}
		else
		{
			isInitOK = true;
		}
	}
	return isInitOK;
}

int ProviderUdpSSL::open()
{
	int retval = -1;
	_isDeviceReady = false;

	Info(_log, "Open UDP SSL streaming to %s port: %d", QSTRING_CSTR(_address.toString()), _ssl_port);

	if ( !initNetwork() )
	{
		this->setInError( "UDP SSL Network error!" );
	}
	else
	{
		// Everything is OK -> enable device
		Info(_log, "Stream UDP SSL data to %s port: %d", QSTRING_CSTR(_address.toString()), _ssl_port);
		_isDeviceReady = true;
		retval = 0;
	}
	return retval;
}

int ProviderUdpSSL::close()
{
	int retval = 0;
	_isDeviceReady = false;

	Debug(_log, "Close SSL UDP-device: %s", QSTRING_CSTR(this->getActiveDeviceType()));
	stopConnection();

	// Everything is OK -> device is closed
	return retval;
}

const int *ProviderUdpSSL::getCiphersuites() const
{
	return mbedtls_ssl_list_ciphersuites();
}

bool ProviderUdpSSL::initNetwork()
{
	if ((!_isDeviceReady || _streamPaused) && _streamReady)
	{
		stopConnection();
	}

	return initConnection();
}

bool ProviderUdpSSL::initConnection()
{
	if (_streamReady)
	{
		return true;
	}

	mbedtls_net_init(&client_fd);
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&conf);
	mbedtls_x509_crt_init(&cacert);

	if (setupStructure())
	{
		_streamReady = true;
		_streamPaused = false;
		return true;
	}

	return false;
}

bool ProviderUdpSSL::seedingRNG()
{
	mbedtls_entropy_init(&entropy);

	QByteArray customDataArray = _custom.toLocal8Bit();
	const char* customData = customDataArray.constData();

	int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func,
									&entropy, reinterpret_cast<const unsigned char*>(customData),
									 std::min(strlen(customData), static_cast<size_t>(MBEDTLS_CTR_DRBG_MAX_SEED_INPUT)));

	if (ret != 0)
	{
		Error(_log, "%s", QSTRING_CSTR(QString("mbedtls_ctr_drbg_seed FAILED %1").arg(errorMsg(ret))));
		return false;
	}
	return true;
}

bool ProviderUdpSSL::setupStructure()
{
	int transport = ( _transport_type == "DTLS" ) ? MBEDTLS_SSL_TRANSPORT_DATAGRAM : MBEDTLS_SSL_TRANSPORT_STREAM;

	int ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, transport, MBEDTLS_SSL_PRESET_DEFAULT);

	if (ret != 0)
	{
		Error(_log, "%s", QSTRING_CSTR(QString("mbedtls_ssl_config_defaults FAILED %1").arg(errorMsg(ret))));
		return false;
	}

	const int * ciphersuites = getCiphersuites();

	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_ca_chain(&conf, &cacert, nullptr);

	mbedtls_ssl_conf_handshake_timeout(&conf, _handshake_timeout_min, _handshake_timeout_max);

	mbedtls_ssl_conf_ciphersuites(&conf, ciphersuites);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

	if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
	{
		Error(_log, "%s", QSTRING_CSTR(QString("mbedtls_ssl_setup FAILED %1").arg(errorMsg(ret))));
		return false;
	}

	return setupPSK();
}

bool ProviderUdpSSL::startConnection()
{
	mbedtls_ssl_session_reset(&ssl);

	int ret = mbedtls_net_connect(&client_fd, _address.toString().toUtf8(), std::to_string(_ssl_port).c_str(), MBEDTLS_NET_PROTO_UDP);

	if (ret != 0)
	{
		Error(_log, "%s", QSTRING_CSTR(QString("mbedtls_net_connect FAILED %1").arg(errorMsg(ret))));
		return false;
	}

	mbedtls_ssl_set_bio(&ssl, &client_fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);
	mbedtls_ssl_set_timer_cb(&ssl, &timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);

	return startSSLHandshake();
}

bool ProviderUdpSSL::setupPSK()
{
	QByteArray pskRawArray = QByteArray::fromHex(_psk.toUtf8());
	QByteArray pskIdRawArray = _psk_identity.toUtf8();

	int ret = mbedtls_ssl_conf_psk( &conf,
									reinterpret_cast<const unsigned char*> (pskRawArray.constData()),
									pskRawArray.length(),
									reinterpret_cast<const unsigned char*> (pskIdRawArray.constData()),
									pskIdRawArray.length());

	if (ret != 0)
	{
		Error(_log, "%s", QSTRING_CSTR(QString("mbedtls_ssl_conf_psk FAILED %1").arg(errorMsg(ret))));
		return false;
	}
	return true;
}

bool ProviderUdpSSL::startSSLHandshake()
{
	int ret = 0;
	for (int attempt = 1; attempt <= _handshake_attempts; ++attempt)
	{
		do
		{
			ret = mbedtls_ssl_handshake(&ssl);
		} while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

		if (ret == 0)
		{
			break;
		}

		Warning(_log, "%s", QSTRING_CSTR(QString("mbedtls_ssl_handshake attempt %1/%2 FAILED. Reason: %3").arg(attempt).arg(_handshake_attempts).arg(errorMsg(ret))));
		QThread::msleep(200);
	}

	if (ret != 0)
	{
		Error(_log, "%s", QSTRING_CSTR(QString("mbedtls_ssl_handshake FAILED %1").arg(errorMsg(ret))));
		return false;
	}

	if (mbedtls_ssl_get_verify_result(&ssl) != 0)
	{
		Error(_log, "SSL certificate verification failed!");
		return false;
	}

	return true;
}

void ProviderUdpSSL::stopConnection()
{
	if (_streamReady)
	{
		closeSSLNotify();
		freeSSLConnection();
		_streamReady = false;
	}
}

void ProviderUdpSSL::freeSSLConnection()
{
	try
	{
		Debug(_log, "Release mbedtls");
		mbedtls_ssl_session_reset(&ssl);
		mbedtls_net_free(&client_fd);
		mbedtls_ssl_free(&ssl);
		mbedtls_ssl_config_free(&conf);
		mbedtls_x509_crt_free(&cacert);
	}
	catch (std::exception &e)
	{
		Error(_log, "%s", QSTRING_CSTR(QString("SSL Connection clean-up Error: %s").arg(e.what())));
	}
	catch (...)
	{
		Error(_log, "SSL Connection clean-up Error: <unknown>");
	}
}

void ProviderUdpSSL::writeBytes(unsigned int size, const uint8_t* data, bool flush)
{
	if (!_streamReady || _streamPaused)
	{
		return;
	}

	if (!_streamReady || _streamPaused)
	{
		return;
	}

	_streamPaused = flush;

	int ret = 0;

	do
	{
		ret = mbedtls_ssl_write(&ssl, data, size);
	} while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	if (ret <= 0)
	{
		Error(_log, "Error while writing UDP SSL stream updates. mbedtls_ssl_write returned: %s", QSTRING_CSTR(errorMsg(ret)));

		if (_streamReady)
		{
			stopConnection();
			disable();

			startEnableAttemptsTimer();
		}
	}
}

QString ProviderUdpSSL::errorMsg(int ret)
{
	char error_buf[1024];
	mbedtls_strerror(ret, error_buf, 1024);

	return QString("Last error was: code = %1, description = %2").arg(ret).arg(error_buf);
}

void ProviderUdpSSL::closeSSLNotify()
{
	/* No error checking, the connection might be closed already */
	while (mbedtls_ssl_close_notify(&ssl) == MBEDTLS_ERR_SSL_WANT_WRITE)
	{
	}
}
