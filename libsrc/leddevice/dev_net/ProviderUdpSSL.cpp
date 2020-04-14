
// STL includes
#include <cstdio>
#include <exception>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

#include <QHostInfo>

// Local Hyperion includes
#include "ProviderUdpSSL.h"

ProviderUdpSSL::ProviderUdpSSL()
	: LedDevice()
	, client_fd()
	, entropy()
	, ssl()
	, conf()
	, ctr_drbg()
	, timer()
	, _transport_type("DTLS")
	, _custom("dtls_client")
	, _address("127.0.0.1")
	, _defaultHost("127.0.0.1")
	, _port(1)
	, _ssl_port(1)
	, _server_name("")
	, _psk("")
	, _psk_identity("")
	, _retry_left(MAX_RETRY)
	, _handshake_attempts(5)
	, _stopConnection(true)
{
	_deviceReady = false;
	_latchTime_ms = 1;
}

ProviderUdpSSL::~ProviderUdpSSL()
{
}

bool ProviderUdpSSL::init(const QJsonObject &deviceConfig)
{
	bool isInitOK = LedDevice::init(deviceConfig);

	_debugStreamer = deviceConfig["debugStreamer"].toBool(false);
	_debugLevel = deviceConfig["debugLevel"].toInt(0);

	//PSK Pre Shared Key
	_psk = deviceConfig["psk"].toString("");
	_psk_identity = deviceConfig["psk_identity"].toString("");
	_port = deviceConfig["sslport"].toInt(2100);
	_server_name = deviceConfig["servername"].toString("");

	if( deviceConfig.contains("retry_left") )		_retry_left         = deviceConfig["retry_left"].toInt(MAX_RETRY);
	if( deviceConfig.contains("hs_attempts") )		_handshake_attempts = deviceConfig["hs_attempts"].toInt(5);
	if( deviceConfig.contains("seed_custom") )		_custom             = deviceConfig["seed_custom"].toString("dtls_client");
	if( deviceConfig.contains("transport_type") )	_transport_type     = deviceConfig["transport_type"].toString("DTLS");


	#define DEBUG_LEVEL _debugLevel

	QString host = deviceConfig["host"].toString(_defaultHost);

	if ( _address.setAddress(host) )
	{
		Debug( _log, "Successfully parsed %s as an ip address.", host.toStdString().c_str() );
	}
	else
	{
		Debug( _log, "Failed to parse [%s] as an ip address.", host.toStdString().c_str() );
		QHostInfo info = QHostInfo::fromName(host);
		if ( info.addresses().isEmpty() )
		{
			Debug( _log, "Failed to parse [%s] as a hostname.", host.toStdString().c_str() );
			QString errortext = QString("Invalid target address [%1]!").arg(host);
			this->setInError( errortext );
			isInitOK = false;
		}
		else
		{
			Debug( _log, "Successfully parsed %s as a hostname.", host.toStdString().c_str() );
			_address = info.addresses().first();
		}
	}

	int config_port = deviceConfig["sslport"].toInt(_port);

	if ( config_port <= 0 || config_port > MAX_PORT_SSL )
	{
		QString errortext = QString ("Invalid target port [%1]!").arg(config_port);
		this->setInError( errortext );
		isInitOK = false;
	}
	else
	{
		_ssl_port = config_port;
		Debug( _log, "UDP SSL using %s:%u", _address.toString().toStdString().c_str() , _ssl_port );
	}
	return isInitOK;
}

int ProviderUdpSSL::open()
{
	int retval = -1;
	QString errortext;
	_deviceReady = false;

	if ( init(_devConfig) )
	{
		if ( !initNetwork() )
		{
			this->setInError( "UDP SSL Network error!" );
		}
		else
		{
			// Everything is OK -> enable device
			_deviceReady = true;
			setEnable(true);
			retval = 0;
		}
	}
	return retval;
}

void ProviderUdpSSL::close()
{
	LedDevice::close();
	closeSSLConnection();
}

void ProviderUdpSSL::closeSSLConnection()
{
	if( _deviceReady && !_stopConnection )
	{
		closeSSLNotify();
		freeSSLConnection();
	}
}

const int *ProviderUdpSSL::getCiphersuites()
{
	return mbedtls_ssl_list_ciphersuites();
}

void ProviderUdpSSL::log(const char* msg)
{
	log( msg, "debug" );
}

void ProviderUdpSSL::log(QString msg)
{
	log( msg, "debug" );
}

void ProviderUdpSSL::log(QString msg, const char* errorType)
{
	log( msg.toStdString().c_str(), errorType );
}

void ProviderUdpSSL::log(const char* msg, const char* errorType)
{
	if( _debugStreamer )
	{
		if( strcmp("debug", errorType) == 0 )	Debug( _log, "%s", msg );
		if( strcmp("warning", errorType) == 0 )	Warning( _log, "%s", msg );
		if( strcmp("error", errorType) == 0 )	Error( _log, "%s", msg );
	}
}

bool ProviderUdpSSL::initNetwork()
{
	bool isInitOK = true;

	log( "init SSL Network..." );

	QMutexLocker locker(&_hueMutex);

	log( "init SSL Network -> initConnection" );

	if(!initConnection()) isInitOK = false;

	log( "init SSL Network -> startUPDConnection" );

	if(!startUPDConnection()) isInitOK = false;

	log( "init SSL Network -> startSSLHandshake" );

	if(!startSSLHandshake()) isInitOK = false;

	log( "init SSL Network...ok" );

	_stopConnection = false;

	return isInitOK;
}

bool ProviderUdpSSL::initConnection()
{
	mbedtls_net_init(&client_fd);
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&conf);
	mbedtls_x509_crt_init(&cacert);
	mbedtls_ctr_drbg_init(&ctr_drbg);
	if(!seedingRNG()) return false;
	return setupStructure();
}

bool ProviderUdpSSL::seedingRNG()
{
	int ret;

	log( "Seeding the random number generator..." );

	mbedtls_entropy_init(&entropy);

	log( "Set mbedtls_ctr_drbg_seed..." );

	const char* custom = _custom.toStdString().c_str();

	if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, reinterpret_cast<const unsigned char *>(custom), strlen(custom))) != 0)
	{
		if(_debugStreamer) mbedtls_printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
		return false;
	}

	log( "Seeding the random number generator...ok" );

	return true;
}

bool ProviderUdpSSL::setupStructure()
{
	int ret;

	log( QString( "Setting up the %1 structure").arg( _transport_type ) );

	//TLS  MBEDTLS_SSL_TRANSPORT_STREAM
	//DTLS MBEDTLS_SSL_TRANSPORT_DATAGRAM

	int transport = ( _transport_type == "DTLS" ) ? MBEDTLS_SSL_TRANSPORT_DATAGRAM : MBEDTLS_SSL_TRANSPORT_STREAM;

	if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, transport, MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
	{
		log( QString("mbedtls_ssl_config_defaults FAILED %1").arg(ret), "error" );
		return false;
	}

	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	//mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
	//mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	const int * ciphersuites = getCiphersuites();

	int s = sizeof(ciphersuites) / sizeof(ciphersuites[0]);

	QString cipher_values;
	for(int i=0; i<s; i++)
	{
		cipher_values.append(QString::number(ciphersuites[i]));
	}

	log( QString("used ciphersuites value: %1").arg(cipher_values), "debug" );

	mbedtls_ssl_conf_ciphersuites(&conf, ciphersuites);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

#if DEBUG_LEVEL > 0
	mbedtls_ssl_conf_dbg(&conf, ProviderUdpSSLDebug, NULL);
	mbedtls_debug_set_threshold(DEBUG_LEVEL);
#endif

	mbedtls_ssl_conf_handshake_timeout(&conf, 400, 1000);

	if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
	{
		log( QString("mbedtls_ssl_setup FAILED %1").arg(ret), "error" );
		return false;
	}

	if ((ret = mbedtls_ssl_set_hostname(&ssl, _server_name.toStdString().c_str())) != 0)
	{
		log( QString("mbedtls_ssl_set_hostname FAILED %1").arg(ret), "error" );
		return false;
	}

	log( "Setting up the structure...ok" );

	return true;
}

bool ProviderUdpSSL::startUPDConnection()
{
	int ret;

	mbedtls_ssl_session_reset(&ssl);

	if(!setupPSK()) return false;

	log( QString("Connecting to udp %1:%2").arg( _address.toString() ).arg( _ssl_port ) );

	if ((ret = mbedtls_net_connect( &client_fd, _address.toString().toUtf8(), std::to_string(_ssl_port).c_str(), MBEDTLS_NET_PROTO_UDP)) != 0)
	{
		log( QString("mbedtls_net_connect FAILED %1").arg(ret), "error" );
		return false;
	}

	mbedtls_ssl_set_bio(&ssl, &client_fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);
	mbedtls_ssl_set_timer_cb(&ssl, &timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);

	log( "Connecting...ok" );

	return true;
}

bool ProviderUdpSSL::setupPSK()
{
	int ret;

	QByteArray pskArray = _psk.toUtf8();
	QByteArray pskRawArray = QByteArray::fromHex(pskArray);

	QByteArray pskIdArray = _psk_identity.toUtf8();
	QByteArray pskIdRawArray = pskIdArray;

	if (0 != (ret = mbedtls_ssl_conf_psk( &conf, ( const unsigned char* ) pskRawArray.data(), pskRawArray.length() * sizeof(char), reinterpret_cast<const unsigned char *> ( pskIdRawArray.data() ), pskIdRawArray.length() * sizeof(char) ) ) )
	{
		log( QString("mbedtls_ssl_conf_psk FAILED %1").arg(ret), "error" );
		return false;
	}

	return true;
}

bool ProviderUdpSSL::startSSLHandshake()
{
	int ret;

	log( "Performing the SSL/TLS handshake..." );

	for (int attempt = 1; attempt <= _handshake_attempts; ++attempt)
	{
		log( QString("handshake attempt %1/%2").arg( attempt ).arg( _handshake_attempts ) );

		do
		{
			ret = mbedtls_ssl_handshake(&ssl);
		}
		while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

		if (ret == 0) break;

		QThread::msleep(200);
	}

	if (ret != 0)
	{
		log( QString("mbedtls_ssl_handshake FAILED %1").arg(ret), "error" );

		handleReturn(ret);

		Error(_log, "UDP SSL Connection failed!");

#if DEBUG_LEVEL > 0
#ifdef MBEDTLS_ERROR_C
		char error_buf[100];
		mbedtls_strerror(ret, error_buf, 100);
		mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf);
#endif
#endif
		return false;
	}

	log( "Performing the SSL/TLS handshake...ok" );

	return true;
}

void ProviderUdpSSL::freeSSLConnection()
{
	log( "SSL Connection cleanup..." );

	_stopConnection = true;

	try
	{
		mbedtls_ssl_session_reset(&ssl);
		mbedtls_net_free(&client_fd);
		mbedtls_ssl_free(&ssl);
		mbedtls_ssl_config_free(&conf);
		mbedtls_x509_crt_free(&cacert);
		mbedtls_ctr_drbg_free(&ctr_drbg);
		mbedtls_entropy_free(&entropy);
		log( "SSL Connection cleanup...ok" );
	}
	catch (std::exception &e)
	{
		log( QString("SSL Connection cleanup Error: %s").arg( e.what() ) );
	}
	catch (...)
	{
		log( "SSL Connection cleanup Error: <unknown>" );
	}
}

void ProviderUdpSSL::writeBytes(const unsigned size, const unsigned char * data)
{
	if( _stopConnection ) return;

	QMutexLocker locker(&_hueMutex);

	int ret;

	do
	{
		ret = mbedtls_ssl_write(&ssl, data, size);
	}
	while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	if (ret <= 0)
	{
		handleReturn(ret);
	}
}

void ProviderUdpSSL::handleReturn(int ret)
{
	bool closeNotify = false;
	bool gotoExit = false;

	switch (ret)
	{
		case MBEDTLS_ERR_SSL_TIMEOUT:
			log( "timeout", "warning" );
			if (_retry_left-- > 0) return;
			gotoExit = true;
			break;

		case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
			log( "SSL Connection was closed gracefully", "warning" );
			ret = 0;
			closeNotify = true;
			break;
		default:
			log( QString("mbedtls_ssl_read returned %1").arg( errorMsg(ret) ), "warning" );
			gotoExit = true;
	}

	if (closeNotify)
	{
		closeSSLNotify();
		gotoExit = true;
	}

	if (gotoExit)
	{
		log( "Exit SSL connection" );
		_stopConnection = true;
	}
}

QString ProviderUdpSSL::errorMsg(int ret) {

	QString msg = "";

	switch (ret)
	{
#if defined(MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE)
		case MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE:
			msg = "MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE               -0x7080";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_BAD_INPUT_DATA)
		case MBEDTLS_ERR_SSL_BAD_INPUT_DATA:
			msg = "MBEDTLS_ERR_SSL_BAD_INPUT_DATA                     -0x7100";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_INVALID_MAC)
		case MBEDTLS_ERR_SSL_INVALID_MAC:
			msg = "MBEDTLS_ERR_SSL_INVALID_MAC                        -0x7180";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_INVALID_RECORD)
		case MBEDTLS_ERR_SSL_INVALID_RECORD:
			msg = "MBEDTLS_ERR_SSL_INVALID_RECORD                     -0x7200";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_CONN_EOF)
		case MBEDTLS_ERR_SSL_CONN_EOF:
			msg = "MBEDTLS_ERR_SSL_CONN_EOF                           -0x7280";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_UNKNOWN_CIPHER)
		case MBEDTLS_ERR_SSL_UNKNOWN_CIPHER:
			msg = "MBEDTLS_ERR_SSL_UNKNOWN_CIPHER                     -0x7300";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_NO_CIPHER_CHOSEN)
		case MBEDTLS_ERR_SSL_NO_CIPHER_CHOSEN:
			msg = "MBEDTLS_ERR_SSL_NO_CIPHER_CHOSEN                   -0x7380";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_NO_RNG)
		case MBEDTLS_ERR_SSL_NO_RNG:
			msg = "MBEDTLS_ERR_SSL_NO_RNG                             -0x7400";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_NO_CLIENT_CERTIFICATE)
		case MBEDTLS_ERR_SSL_NO_CLIENT_CERTIFICATE:
			msg = "MBEDTLS_ERR_SSL_NO_CLIENT_CERTIFICATE              -0x7480";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_CERTIFICATE_TOO_LARGE)
		case MBEDTLS_ERR_SSL_CERTIFICATE_TOO_LARGE:
			msg = "MBEDTLS_ERR_SSL_CERTIFICATE_TOO_LARGE              -0x7500";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_CERTIFICATE_REQUIRED)
		case MBEDTLS_ERR_SSL_CERTIFICATE_REQUIRED:
			msg = "MBEDTLS_ERR_SSL_CERTIFICATE_REQUIRED               -0x7580";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_PRIVATE_KEY_REQUIRED)
		case MBEDTLS_ERR_SSL_PRIVATE_KEY_REQUIRED:
			msg = "MBEDTLS_ERR_SSL_PRIVATE_KEY_REQUIRED               -0x7600";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_CA_CHAIN_REQUIRED)
		case MBEDTLS_ERR_SSL_CA_CHAIN_REQUIRED:
			msg = "MBEDTLS_ERR_SSL_CA_CHAIN_REQUIRED                  -0x7680";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE)
		case MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE:
			msg = "MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE                 -0x7700";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE)
		case MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE:
			msg = "MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE                -0x7780";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_PEER_VERIFY_FAILED)
		case MBEDTLS_ERR_SSL_PEER_VERIFY_FAILED:
			msg = "MBEDTLS_ERR_SSL_PEER_VERIFY_FAILED                 -0x7800";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_BAD_HS_CLIENT_HELLO)
		case MBEDTLS_ERR_SSL_BAD_HS_CLIENT_HELLO:
			msg = "MBEDTLS_ERR_SSL_BAD_HS_CLIENT_HELLO                -0x7900";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_BAD_HS_SERVER_HELLO)
		case MBEDTLS_ERR_SSL_BAD_HS_SERVER_HELLO:
			msg = "MBEDTLS_ERR_SSL_BAD_HS_SERVER_HELLO                -0x7980";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_BAD_HS_CERTIFICATE)
		case MBEDTLS_ERR_SSL_BAD_HS_CERTIFICATE:
			msg = "MBEDTLS_ERR_SSL_BAD_HS_CERTIFICATE                 -0x7A00";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_BAD_HS_CERTIFICATE_REQUEST)
		case MBEDTLS_ERR_SSL_BAD_HS_CERTIFICATE_REQUEST:
			msg = "MBEDTLS_ERR_SSL_BAD_HS_CERTIFICATE_REQUEST         -0x7A80";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_BAD_HS_SERVER_KEY_EXCHANGE)
		case MBEDTLS_ERR_SSL_BAD_HS_SERVER_KEY_EXCHANGE:
			msg = "MBEDTLS_ERR_SSL_BAD_HS_SERVER_KEY_EXCHANGE         -0x7B00";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_BAD_HS_SERVER_HELLO_DONE)
		case MBEDTLS_ERR_SSL_BAD_HS_SERVER_HELLO_DONE:
			msg = "MBEDTLS_ERR_SSL_BAD_HS_SERVER_HELLO_DONE           -0x7B80";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE)
		case MBEDTLS_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE:
			msg = "MBEDTLS_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE         -0x7C00";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE_RP)
		case MBEDTLS_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE_RP:
			msg = "MBEDTLS_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE_RP      -0x7C80";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE_CS)
		case MBEDTLS_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE_CS:
			msg = "MBEDTLS_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE_CS      -0x7D00";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_BAD_HS_CERTIFICATE_VERIFY)
		case MBEDTLS_ERR_SSL_BAD_HS_CERTIFICATE_VERIFY:
			msg = "MBEDTLS_ERR_SSL_BAD_HS_CERTIFICATE_VERIFY          -0x7D80";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_BAD_HS_CHANGE_CIPHER_SPEC)
		case MBEDTLS_ERR_SSL_BAD_HS_CHANGE_CIPHER_SPEC:
			msg = "MBEDTLS_ERR_SSL_BAD_HS_CHANGE_CIPHER_SPEC          -0x7E00";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_BAD_HS_FINISHED)
		case MBEDTLS_ERR_SSL_BAD_HS_FINISHED:
			msg = "MBEDTLS_ERR_SSL_BAD_HS_FINISHED                    -0x7E80";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_ALLOC_FAILED)
		case MBEDTLS_ERR_SSL_ALLOC_FAILED:
			msg = "MBEDTLS_ERR_SSL_ALLOC_FAILED                       -0x7F00";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_HW_ACCEL_FAILED)
		case MBEDTLS_ERR_SSL_HW_ACCEL_FAILED:
			msg = "MBEDTLS_ERR_SSL_HW_ACCEL_FAILED                    -0x7F80";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_HW_ACCEL_FALLTHROUGH)
		case MBEDTLS_ERR_SSL_HW_ACCEL_FALLTHROUGH:
			msg = "MBEDTLS_ERR_SSL_HW_ACCEL_FALLTHROUGH               -0x6F80";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_COMPRESSION_FAILED)
		case MBEDTLS_ERR_SSL_COMPRESSION_FAILED:
			msg = "MBEDTLS_ERR_SSL_COMPRESSION_FAILED                 -0x6F00";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_BAD_HS_PROTOCOL_VERSION)
		case MBEDTLS_ERR_SSL_BAD_HS_PROTOCOL_VERSION:
			msg = "MBEDTLS_ERR_SSL_BAD_HS_PROTOCOL_VERSION            -0x6E80";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_BAD_HS_NEW_SESSION_TICKET)
		case MBEDTLS_ERR_SSL_BAD_HS_NEW_SESSION_TICKET:
			msg = "MBEDTLS_ERR_SSL_BAD_HS_NEW_SESSION_TICKET          -0x6E00";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_SESSION_TICKET_EXPIRED)
		case MBEDTLS_ERR_SSL_SESSION_TICKET_EXPIRED:
			msg = "MBEDTLS_ERR_SSL_SESSION_TICKET_EXPIRED             -0x6D80";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_PK_TYPE_MISMATCH)
		case MBEDTLS_ERR_SSL_PK_TYPE_MISMATCH:
			msg = "MBEDTLS_ERR_SSL_PK_TYPE_MISMATCH                   -0x6D00";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_UNKNOWN_IDENTITY)
		case MBEDTLS_ERR_SSL_UNKNOWN_IDENTITY:
			msg = "MBEDTLS_ERR_SSL_UNKNOWN_IDENTITY                   -0x6C80";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_INTERNAL_ERROR)
		case MBEDTLS_ERR_SSL_INTERNAL_ERROR:
			msg = "MBEDTLS_ERR_SSL_INTERNAL_ERROR                     -0x6C00";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_COUNTER_WRAPPING)
		case MBEDTLS_ERR_SSL_COUNTER_WRAPPING:
			msg = "MBEDTLS_ERR_SSL_COUNTER_WRAPPING                   -0x6B80";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_WAITING_SERVER_HELLO_RENEGO)
		case MBEDTLS_ERR_SSL_WAITING_SERVER_HELLO_RENEGO:
			msg = "MBEDTLS_ERR_SSL_WAITING_SERVER_HELLO_RENEGO        -0x6B00";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED)
		case MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED:
			msg = "MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED              -0x6A80";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL)
		case MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL:
			msg = "MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL                   -0x6A00";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_NO_USABLE_CIPHERSUITE)
		case MBEDTLS_ERR_SSL_NO_USABLE_CIPHERSUITE:
			msg = "MBEDTLS_ERR_SSL_NO_USABLE_CIPHERSUITE              -0x6980";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_WANT_READ)
		case MBEDTLS_ERR_SSL_WANT_READ:
			msg = "MBEDTLS_ERR_SSL_WANT_READ                          -0x6900";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_WANT_WRITE)
		case MBEDTLS_ERR_SSL_WANT_WRITE:
			msg = "MBEDTLS_ERR_SSL_WANT_WRITE                         -0x6880";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_CLIENT_RECONNECT)
		case MBEDTLS_ERR_SSL_CLIENT_RECONNECT:
			msg = "MBEDTLS_ERR_SSL_CLIENT_RECONNECT                   -0x6780";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_UNEXPECTED_RECORD)
		case MBEDTLS_ERR_SSL_UNEXPECTED_RECORD:
			msg = "MBEDTLS_ERR_SSL_UNEXPECTED_RECORD                  -0x6700";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_NON_FATAL)
		case MBEDTLS_ERR_SSL_NON_FATAL:
			msg = "MBEDTLS_ERR_SSL_NON_FATAL                          -0x6680";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_INVALID_VERIFY_HASH)
		case MBEDTLS_ERR_SSL_INVALID_VERIFY_HASH:
			msg = "MBEDTLS_ERR_SSL_INVALID_VERIFY_HASH                -0x6600";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_CONTINUE_PROCESSING)
		case MBEDTLS_ERR_SSL_CONTINUE_PROCESSING:
			msg = "MBEDTLS_ERR_SSL_CONTINUE_PROCESSING                -0x6580";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS)
		case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
			msg = "MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS                  -0x6500";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_EARLY_MESSAGE)
		case MBEDTLS_ERR_SSL_EARLY_MESSAGE:
			msg = "MBEDTLS_ERR_SSL_EARLY_MESSAGE                      -0x6480";
			break;
#endif
#if defined(MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS)
		case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS:
			msg = "MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS                 -0x7000";
			break;
#endif
		default:
			msg = QString::number(ret);
	}

	return msg;
}

void ProviderUdpSSL::closeSSLNotify()
{
	int ret;

	log( "Closing SSL connection..." );
	/* No error checking, the connection might be closed already */
	do
	{
		ret = mbedtls_ssl_close_notify(&ssl);
	}
	while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	log( "SSL Connection successful closed" );
}
