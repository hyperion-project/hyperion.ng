// Local-Hyperion includes
#include "LedDeviceWled.h"

#include <chrono>

#include <utils/QStringUtils.h>
#include <utils/WaitTime.h>

// mDNS discover
#ifdef ENABLE_MDNS
#include <mdns/MdnsBrowser.h>
#include <mdns/MdnsServiceRegister.h>
#endif
#include <utils/NetUtils.h>
#include <utils/version.hpp>

// Constants
namespace {

const bool verbose = false;

// Configuration settings
const char CONFIG_HOST[] = "host";
const char CONFIG_STREAM_PROTOCOL[] = "streamProtocol";
const char CONFIG_RESTORE_STATE[] = "restoreOriginalState";
const char CONFIG_BRIGHTNESS[] = "brightness";
const char CONFIG_BRIGHTNESS_OVERWRITE[] = "overwriteBrightness";
const char CONFIG_SYNC_OVERWRITE[] = "overwriteSync";

const char DEFAULT_STREAM_PROTOCOL[] = "DDP";

// UDP-RAW
const int UDP_STREAM_DEFAULT_PORT = 19446;
const int UDP_MAX_LED_NUM = 490;

// DDP
const char WLED_VERSION_DDP[] = "0.11.0";

// WLED JSON-API elements
const int API_DEFAULT_PORT = -1; //Use default port per communication scheme

const char API_BASE_PATH[] = "/json/";
//const char API_PATH_INFO[] = "info";
const char API_PATH_STATE[] = "state";

// List of State Information
const char STATE_ON[] = "on";
const char STATE_VALUE_TRUE[] = "true";
const char STATE_VALUE_FALSE[] = "false";
const char STATE_LIVE[] = "live";

const bool DEFAULT_IS_RESTORE_STATE = false;
const bool DEFAULT_IS_BRIGHTNESS_OVERWRITE = true;
const int BRI_MAX = 255;
const bool DEFAULT_IS_SYNC_OVERWRITE = true;

constexpr std::chrono::milliseconds DEFAULT_IDENTIFY_TIME{ 2000 };

} //End of constants

LedDeviceWled::LedDeviceWled(const QJsonObject &deviceConfig)
	: ProviderUdp(deviceConfig), LedDeviceUdpDdp(deviceConfig), LedDeviceUdpRaw(deviceConfig)
	  ,_restApi(nullptr)
	  ,_apiPort(API_DEFAULT_PORT)
	  ,_isBrightnessOverwrite(DEFAULT_IS_BRIGHTNESS_OVERWRITE)
	  ,_brightness (BRI_MAX)
	  ,_isSyncOverwrite(DEFAULT_IS_SYNC_OVERWRITE)
	  ,_originalStateUdpnSend(false)
	  ,_originalStateUdpnRecv(true)
	  ,_isStreamDDP(true)
{
#ifdef ENABLE_MDNS
	QMetaObject::invokeMethod(&MdnsBrowser::getInstance(), "browseForServiceType",
							   Qt::QueuedConnection, Q_ARG(QByteArray, MdnsServiceRegister::getServiceType(_activeDeviceType)));
#endif
}

LedDeviceWled::~LedDeviceWled()
{
	delete _restApi;
	_restApi = nullptr;
}

LedDevice* LedDeviceWled::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceWled(deviceConfig);
}

bool LedDeviceWled::init(const QJsonObject &deviceConfig)
{
	bool isInitOK {false};

	QString streamProtocol = _devConfig[CONFIG_STREAM_PROTOCOL].toString(DEFAULT_STREAM_PROTOCOL);

	if (streamProtocol != DEFAULT_STREAM_PROTOCOL)
	{
		_isStreamDDP = false;
	}
	Debug(_log, "Stream protocol   : %s", QSTRING_CSTR(streamProtocol));
	Debug(_log, "Stream DDP        : %d", _isStreamDDP);

	if (_isStreamDDP)
	{
		LedDeviceUdpDdp::init(deviceConfig);
	}
	else
	{
		_devConfig["port"] = UDP_STREAM_DEFAULT_PORT;
		LedDeviceUdpRaw::init(_devConfig);
	}

	if (!_isDeviceInError)
	{
		_apiPort = API_DEFAULT_PORT;
		_isRestoreOrigState = _devConfig[CONFIG_RESTORE_STATE].toBool(DEFAULT_IS_RESTORE_STATE);
		_isSyncOverwrite = _devConfig[CONFIG_SYNC_OVERWRITE].toBool(DEFAULT_IS_SYNC_OVERWRITE);
		_isBrightnessOverwrite = _devConfig[CONFIG_BRIGHTNESS_OVERWRITE].toBool(DEFAULT_IS_BRIGHTNESS_OVERWRITE);
		_brightness = _devConfig[CONFIG_BRIGHTNESS].toInt(BRI_MAX);

		Debug(_log, "RestoreOrigState  : %d", _isRestoreOrigState);
		Debug(_log, "Overwrite Sync.   : %d", _isSyncOverwrite);
		Debug(_log, "Overwrite Brightn.: %d", _isBrightnessOverwrite);
		Debug(_log, "Set Brightness to : %d", _brightness);

		isInitOK = true;
	}

	return isInitOK;
}

bool LedDeviceWled::openRestAPI()
{
	bool isInitOK {true};

	if ( _restApi == nullptr )
	{
		_restApi = new ProviderRestApi(_address.toString(), _apiPort);
		_restApi->setLogger(_log);

		_restApi->setBasePath( API_BASE_PATH );
	}
	else
	{
		_restApi->setHost(_address.toString());
		_restApi->setPort(_apiPort);
	}

	return isInitOK;
}

int LedDeviceWled::open()
{
	int retval = -1;
	_isDeviceReady = false;

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address, _apiPort))
	{
		if ( openRestAPI() )
		{
			if (_isStreamDDP)
			{
				if (LedDeviceUdpDdp::open() == 0)
				{
					// Everything is OK, device is ready
					_isDeviceReady = true;
					retval = 0;
				}
			}
			else
			{
				if (LedDeviceUdpRaw::open() == 0)
				{
					// Everything is OK, device is ready
					_isDeviceReady = true;
					retval = 0;
				}
			}
		}
	}
	return retval;
}

int LedDeviceWled::close()
{
	int retval = -1;
	if (_isStreamDDP)
	{
		retval = LedDeviceUdpDdp::close();
	}
	else
	{
		retval = LedDeviceUdpRaw::close();
	}
	return retval;
}

QString LedDeviceWled::getOnOffRequest(bool isOn) const
{
	QString state = isOn ? STATE_VALUE_TRUE : STATE_VALUE_FALSE;
	return QString( "\"%1\":%2,\"%3\":%4" ).arg( STATE_ON, state).arg( STATE_LIVE, state);
}

QString LedDeviceWled::getBrightnessRequest(int bri) const
{
	return QString( "\"bri\":%1" ).arg(bri);
}

QString LedDeviceWled::getEffectRequest(int effect, int speed) const
{
	return QString( "\"seg\":{\"fx\":%1,\"sx\":%2}" ).arg(effect).arg(speed);
}

QString LedDeviceWled::getLorRequest(int lor) const
{
	return QString( "\"lor\":%1" ).arg(lor);
}

QString LedDeviceWled::getUdpnRequest(bool isSendOn, bool isRecvOn) const
{
	QString send = isSendOn ? STATE_VALUE_TRUE : STATE_VALUE_FALSE;
	QString recv = isRecvOn ? STATE_VALUE_TRUE : STATE_VALUE_FALSE;
	return QString( "\"udpn\":{\"send\":%1,\"recv\":%2}" ).arg(send, recv);
}

bool LedDeviceWled::sendStateUpdateRequest(const QString &request)
{
	bool rc = true;

	_restApi->setPath(API_PATH_STATE);

	httpResponse response1 = _restApi->put(QString("{%1}").arg(request));
	if ( response1.error() )
	{
		rc = false;
	}
	return rc;
}
bool LedDeviceWled::powerOn()
{
	bool on = false;
	if ( _isDeviceReady)
	{
		//Power-on WLED device
		_restApi->setPath(API_PATH_STATE);

		QString cmd = getOnOffRequest(true);

		if ( _isBrightnessOverwrite)
		{
			cmd += "," + getBrightnessRequest(_brightness);
		}

		if (_isSyncOverwrite)
		{
			Debug( _log, "Disable synchronisation with other WLED devices");
			cmd += "," + getUdpnRequest(false, false);
		}

		httpResponse response = _restApi->put(QString("{%1}").arg(cmd));
		if ( response.error() )
		{
			QString errorReason = QString("Power-on request failed with error: '%1'").arg(response.getErrorReason());
			this->setInError ( errorReason );
			on = false;
		}
		else
		{
			on = true;
		}
	}
	return on;
}

bool LedDeviceWled::powerOff()
{
	bool off = true;
	if ( _isDeviceReady)
	{
		// Write a final "Black" to have a defined outcome
		writeBlack();

		//Power-off the WLED device physically
		_restApi->setPath(API_PATH_STATE);

		QString cmd = getOnOffRequest(false);

		if (_isSyncOverwrite)
		{
			Debug( _log, "Restore synchronisation with other WLED devices");
			cmd += "," + getUdpnRequest(_originalStateUdpnSend, _originalStateUdpnRecv);
		}

		httpResponse response = _restApi->put(QString("{%1}").arg(cmd));
		if ( response.error() )
		{
			QString errorReason = QString("Power-off request failed with error: '%1'").arg(response.getErrorReason());
			this->setInError ( errorReason );
			off = false;
		}
	}
	return off;
}

bool LedDeviceWled::storeState()
{
	bool rc = true;

	if ( _isRestoreOrigState || _isSyncOverwrite )
	{
		_restApi->setPath(API_PATH_STATE);

		httpResponse response = _restApi->get();
		if ( response.error() )
		{
			QString errorReason = QString("Storing device state failed with error: '%1'").arg(response.getErrorReason());
			setInError(errorReason);
			rc = false;
		}
		else
		{
			_originalStateProperties = response.getBody().object();
			DebugIf(verbose, _log, "state: [%s]", QString(QJsonDocument(_originalStateProperties).toJson(QJsonDocument::Compact)).toUtf8().constData() );

			QJsonObject udpn = _originalStateProperties.value("udpn").toObject();
			if (!udpn.isEmpty())
			{
				_originalStateUdpnSend = udpn["send"].toBool(false);
				_originalStateUdpnRecv = udpn["recv"].toBool(true);
			}
		}
	}

	return rc;
}

bool LedDeviceWled::restoreState()
{
	bool rc = true;

	if ( _isRestoreOrigState )
	{
		_restApi->setPath(API_PATH_STATE);

		_originalStateProperties[STATE_LIVE] = false;

		httpResponse response = _restApi->put(QString(QJsonDocument(_originalStateProperties).toJson(QJsonDocument::Compact)).toUtf8().constData());

		if ( response.error() )
		{
			Warning (_log, "%s restoring state failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
		}
	}

	return rc;
}

QJsonObject LedDeviceWled::discover(const QJsonObject& /*params*/)
{
	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType );

	QJsonArray deviceList;

#ifdef ENABLE_MDNS
	QString discoveryMethod("mDNS");
	deviceList = MdnsBrowser::getInstance().getServicesDiscoveredJson(
		MdnsServiceRegister::getServiceType(_activeDeviceType),
		MdnsServiceRegister::getServiceNameFilter(_activeDeviceType),
		DEFAULT_DISCOVER_TIMEOUT
		);
	devicesDiscovered.insert("discoveryMethod", discoveryMethod);
#endif
	devicesDiscovered.insert("devices", deviceList);
	DebugIf(verbose, _log, "devicesDiscovered: [%s]", QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	return devicesDiscovered;
}

QJsonObject LedDeviceWled::getProperties(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData() );
	QJsonObject properties;

	_hostName = params[CONFIG_HOST].toString("");
	_apiPort = API_DEFAULT_PORT;

	Info(_log, "Get properties for %s, hostname (%s)", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(_hostName) );

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address, _apiPort))
	{
		if ( openRestAPI() )
		{
			QString filter = params["filter"].toString("");
			_restApi->setPath(filter);

			httpResponse response = _restApi->get();
			if ( response.error() )
			{
				Warning (_log, "%s get properties failed with error: '%s'", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(response.getErrorReason()));
			}

			QJsonObject propertiesDetails = response.getBody().object();

			semver::version currentVersion {""};
			if (currentVersion.setVersion(propertiesDetails.value("ver").toString().toStdString()))
			{
				semver::version ddpVersion{WLED_VERSION_DDP};
				if (currentVersion < ddpVersion)
				{
					Warning(_log, "DDP streaming not supported by your WLED device version [%s], minimum version expected [%s]. Fall back to UDP-Streaming (%d LEDs max)", currentVersion.getVersion().c_str(), ddpVersion.getVersion().c_str(), UDP_MAX_LED_NUM);
					if (!propertiesDetails.isEmpty())
					{
						propertiesDetails.insert("maxLedCount", UDP_MAX_LED_NUM);
					}
				}
				else
				{
					Info(_log, "DDP streaming is supported by your WLED device version [%s]. No limitation in number of LEDs.", currentVersion.getVersion().c_str(), ddpVersion.getVersion().c_str());
				}
			}
			properties.insert("properties", propertiesDetails);
		}

		DebugIf(verbose, _log, "properties: [%s]", QString(QJsonDocument(properties).toJson(QJsonDocument::Compact)).toUtf8().constData() );
	}
	return properties;
}

void LedDeviceWled::identify(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	_hostName = params[CONFIG_HOST].toString("");
	_apiPort = API_DEFAULT_PORT;

	Info(_log, "Identify %s, hostname (%s)", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(_hostName) );

	if (NetUtils::resolveHostToAddress(_log, _hostName, _address, _apiPort))
	{
		if ( openRestAPI() )
		{
			_isRestoreOrigState = true;
			storeState();

			QString request = getOnOffRequest(true) + "," + getLorRequest(1) + "," + getEffectRequest(25);
			sendStateUpdateRequest(request);

			wait(DEFAULT_IDENTIFY_TIME);

			restoreState();
		}
	}
}

int LedDeviceWled::write(const std::vector<ColorRgb> &ledValues)
{
	int rc {0};

	if (_isStreamDDP)
	{
		rc = LedDeviceUdpDdp::write(ledValues);
	}
	else
	{
		rc = LedDeviceUdpRaw::write(ledValues);
	}

	return rc;
}
