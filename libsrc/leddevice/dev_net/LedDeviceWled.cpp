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
const char CONFIG_STAY_ON_AFTER_STREAMING[] = "stayOnAfterStreaming";

const char CONFIG_BRIGHTNESS[] = "brightness";
const char CONFIG_BRIGHTNESS_OVERWRITE[] = "overwriteBrightness";
const char CONFIG_SYNC_OVERWRITE[] = "overwriteSync";

const char CONFIG_STREAM_SEGMENTS[] = "segments";
const char CONFIG_STREAM_SEGMENT_ID[] = "streamSegmentId";
const char CONFIG_SWITCH_OFF_OTHER_SEGMENTS[] = "switchOffOtherSegments";

const char DEFAULT_STREAM_PROTOCOL[] = "DDP";

// UDP-RAW
const int UDP_STREAM_DEFAULT_PORT = 19446;
const int UDP_MAX_LED_NUM = 490;

// Version constraints
const char WLED_VERSION_DDP[] = "0.11.0";
const char WLED_VERSION_SEGMENT_STREAMING[] = "0.13.3";

// WLED JSON-API elements
const int API_DEFAULT_PORT = -1; //Use default port per communication scheme

const char API_BASE_PATH[] = "/json/";
const char API_PATH_STATE[] = "state";
const char API_PATH_INFO[] = "info";

// List of State keys
const char STATE_ON[] = "on";
const char STATE_BRI[] = "bri";
const char STATE_LIVE[] = "live";
const char STATE_LOR[] = "lor";
const char STATE_SEG[] = "seg";
const char STATE_SEG_ID[] = "id";
const char STATE_SEG_LEN[] = "len";
const char STATE_SEG_FX[] = "fx";
const char STATE_SEG_SX[] = "sx";
const char STATE_MAINSEG[] = "mainseg";
const char STATE_UDPN[] = "udpn";
const char STATE_UDPN_SEND[] = "send";
const char STATE_UDPN_RECV[] = "recv";
const char STATE_TRANSITIONTIME_CURRENTCALL[] = "tt";

// List of Info keys
const char INFO_VER[] = "ver";
const char INFO_LIVESEG[] = "liveseg";

//Default state values
const bool DEFAULT_IS_RESTORE_STATE = false;
const bool DEFAULT_IS_STAY_ON_AFTER_STREAMING = false;
const bool DEFAULT_IS_BRIGHTNESS_OVERWRITE = true;
const int BRI_MAX = 255;
const bool DEFAULT_IS_SYNC_OVERWRITE = true;
const int DEFAULT_SEGMENT_ID = -1;
const bool DEFAULT_IS_SWITCH_OFF_OTHER_SEGMENTS = true;

constexpr std::chrono::milliseconds DEFAULT_IDENTIFY_TIME{ 2000 };

} //End of constants

LedDeviceWled::LedDeviceWled(const QJsonObject &deviceConfig)
	: ProviderUdp(deviceConfig), LedDeviceUdpDdp(deviceConfig), LedDeviceUdpRaw(deviceConfig)
	  ,_restApi(nullptr)
	  ,_apiPort(API_DEFAULT_PORT)
	  ,_currentVersion("")
	  ,_isBrightnessOverwrite(DEFAULT_IS_BRIGHTNESS_OVERWRITE)
	  ,_brightness (BRI_MAX)
	  ,_isSyncOverwrite(DEFAULT_IS_SYNC_OVERWRITE)
	  ,_originalStateUdpnSend(false)
	  ,_originalStateUdpnRecv(true)
	  ,_isStreamDDP(true)
	  ,_streamSegmentId(DEFAULT_SEGMENT_ID)
	  ,_isSwitchOffOtherSegments(DEFAULT_IS_SWITCH_OFF_OTHER_SEGMENTS)
	  ,_isStreamToSegment(false)
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
		_isStayOnAfterStreaming = _devConfig[CONFIG_STAY_ON_AFTER_STREAMING].toBool(DEFAULT_IS_STAY_ON_AFTER_STREAMING);
		_isSyncOverwrite = _devConfig[CONFIG_SYNC_OVERWRITE].toBool(DEFAULT_IS_SYNC_OVERWRITE);
		_isBrightnessOverwrite = _devConfig[CONFIG_BRIGHTNESS_OVERWRITE].toBool(DEFAULT_IS_BRIGHTNESS_OVERWRITE);
		_brightness = _devConfig[CONFIG_BRIGHTNESS].toInt(BRI_MAX);

		Debug(_log, "RestoreOrigState  : %d", _isRestoreOrigState);
		Debug(_log, "Overwrite Sync.   : %d", _isSyncOverwrite);
		Debug(_log, "Overwrite Brightn.: %d", _isBrightnessOverwrite);
		Debug(_log, "Set Brightness to : %d", _brightness);


		QJsonObject segments = _devConfig[CONFIG_STREAM_SEGMENTS].toObject();
		_streamSegmentId = segments[CONFIG_STREAM_SEGMENT_ID].toInt(DEFAULT_SEGMENT_ID);

		if (_streamSegmentId > DEFAULT_SEGMENT_ID)
		{
			_isStreamToSegment = true;
		}
		_isSwitchOffOtherSegments = segments[CONFIG_SWITCH_OFF_OTHER_SEGMENTS].toBool(DEFAULT_IS_SWITCH_OFF_OTHER_SEGMENTS);

		Debug(_log, "Stream to one segment: %s", _isStreamToSegment ? "Yes" : "No");
		if (_isStreamToSegment )
		{
			Debug(_log, "Stream to segment [%d]", _streamSegmentId);
			Debug(_log, "Switch-off other segments: %s", _isSwitchOffOtherSegments ? "Yes" : "No");
		}

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

QJsonObject LedDeviceWled::getUdpnObject(bool isSendOn, bool isRecvOn) const
{
	QJsonObject udpnObj
	{
		{STATE_UDPN_SEND, isSendOn},
		{STATE_UDPN_RECV, isRecvOn}
	};
	return udpnObj;
}

QJsonObject LedDeviceWled::getSegmentObject(int segmentId, bool isOn, int brightness) const
{
	QJsonObject segmentObj
	{
		{STATE_SEG_ID, segmentId},
		{STATE_ON, isOn}
	};

	if ( brightness > -1)
	{
		segmentObj.insert(STATE_BRI, brightness);
	}
	return segmentObj;
}

bool LedDeviceWled::sendStateUpdateRequest(const QJsonObject &request, const QString requestType)
{
	bool rc = true;

	_restApi->setPath(API_PATH_STATE);

	httpResponse response = _restApi->put(request);
	if ( response.error() )
	{
		QString errorReason = QString("%1 request failed with error: '%2'").arg(requestType, response.getErrorReason());
		this->setInError ( errorReason );
		rc = false;
	}
	return rc;
}

bool LedDeviceWled::isReadyForSegmentStreaming(semver::version& version) const
{
	bool isReady{false};

	if (version.isValid())
	{
		semver::version segmentStreamingVersion{WLED_VERSION_SEGMENT_STREAMING};
		if (version < segmentStreamingVersion)
		{
			Warning(_log, "Segment streaming not supported by your WLED device version [%s], minimum version expected [%s].", _currentVersion.getVersion().c_str(), segmentStreamingVersion.getVersion().c_str());
		}
		else
		{
			Debug(_log, "Segment streaming is supported by your WLED device version [%s].", _currentVersion.getVersion().c_str());
			isReady = true;
		}
	}
	else
	{
		Error(_log, "Version provided to test for streaming readiness is not valid ");
	}
	return isReady;
}

bool LedDeviceWled::isReadyForDDPStreaming(semver::version& version) const
{
	bool isReady{false};

	if (version.isValid())
	{
		semver::version ddpVersion{WLED_VERSION_DDP};
		if (version < ddpVersion)
		{
			Warning(_log, "DDP streaming not supported by your WLED device version [%s], minimum version expected [%s]. Fall back to UDP-Streaming (%d LEDs max)", _currentVersion.getVersion().c_str(), ddpVersion.getVersion().c_str(), UDP_MAX_LED_NUM);
		}
		else
		{
			Debug(_log, "DDP streaming is supported by your WLED device version [%s]. No limitation in number of LEDs.", _currentVersion.getVersion().c_str());
			isReady = true;
		}
	}
	else
	{
		Error(_log, "Version provided to test for streaming readiness is not valid ");
	}
	return isReady;
}

bool LedDeviceWled::powerOn()
{
	bool on = false;
	if ( _isDeviceReady)
	{
		//Power-on WLED device
		QJsonObject cmd;
		if (_isStreamToSegment)
		{
			if (!isReadyForSegmentStreaming(_currentVersion))
			{
				return false;
			}

			if (_wledInfo[INFO_LIVESEG].toInt() == -1)
			{
				stopEnableAttemptsTimer();
				this->setInError( "Segment streaming configured, but \"Use main segment only\" in WLED Sync Interface configuration is not enabled!", false);
				return false;
			}
			else
			{
				QJsonArray propertiesSegments = _originalStateProperties[STATE_SEG].toArray();

				bool isStreamSegmentIdFound { false };

				QJsonArray segments;
				for (const auto& segmentItem : qAsConst(propertiesSegments))
				{
					QJsonObject segmentObj = segmentItem.toObject();

					int segmentID = segmentObj.value(STATE_SEG_ID).toInt();
					if (segmentID == _streamSegmentId)
					{
						isStreamSegmentIdFound = true;
						int len = segmentObj.value(STATE_SEG_LEN).toInt();
						if (getLedCount() > len)
						{
							QString errorReason = QString("Too many LEDs [%1] configured for segment [%2], which supports maximum [%3] LEDs. Check your WLED setup!").arg(getLedCount()).arg(_streamSegmentId).arg(len);
							this->setInError(errorReason, false);
							return false;
						}
						else
						{
							int brightness{ -1 };
							if (_isBrightnessOverwrite)
							{
								brightness = _brightness;
							}
							segments.append(getSegmentObject(segmentID, true, brightness));
						}
					}
					else
					{
						if (_isSwitchOffOtherSegments)
						{
							segments.append(getSegmentObject(segmentID, false));
						}
					}
				}

				if (!isStreamSegmentIdFound)
				{
					QString errorReason = QString("Segment streaming to segment [%1] configured, but segment does not exist on WLED. Check your WLED setup!").arg(_streamSegmentId);
					this->setInError(errorReason, false);
					return false;
				}

				cmd.insert(STATE_SEG, segments);

				//Set segment to be streamed to
				cmd.insert(STATE_MAINSEG, _streamSegmentId);
			}
		}
		else
		{
			if (_isBrightnessOverwrite)
			{
				cmd.insert(STATE_BRI, _brightness);
			}
		}

		cmd.insert(STATE_LIVE, true);
		cmd.insert(STATE_ON, true);

		if (_isSyncOverwrite)
		{
			Debug( _log, "Disable synchronisation with other WLED devices");
			cmd.insert(STATE_UDPN, getUdpnObject(false, false));
		}

		on = sendStateUpdateRequest(cmd,"Power-on");
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
		QJsonObject cmd;
		if (_isStreamToSegment)
		{
			QJsonArray segments;
			segments.append(getSegmentObject(_streamSegmentId, _isStayOnAfterStreaming));
			cmd.insert(STATE_SEG, segments);
		}

		cmd.insert(STATE_LIVE, false);
		cmd.insert(STATE_TRANSITIONTIME_CURRENTCALL, 0);
		cmd.insert(STATE_ON, _isStayOnAfterStreaming);

		if (_isSyncOverwrite)
		{
			Debug( _log, "Restore synchronisation with other WLED devices");
			cmd.insert(STATE_UDPN, getUdpnObject(_originalStateUdpnSend, _originalStateUdpnRecv));
		}

		off = sendStateUpdateRequest(cmd,"Power-off");
	}
	return off;
}

bool LedDeviceWled::storeState()
{
	bool rc = true;

	if ( _isRestoreOrigState || _isSyncOverwrite || _isStreamToSegment)
	{
		_restApi->setPath("");

		httpResponse response = _restApi->get();
		if ( response.error() )
		{
			QString errorReason = QString("Retrieving device properties failed with error: '%1'").arg(response.getErrorReason());
			setInError(errorReason);
			rc = false;
		}
		else
		{
			_originalStateProperties = response.getBody().object().value(API_PATH_STATE).toObject();
			DebugIf(verbose, _log, "state: [%s]", QString(QJsonDocument(_originalStateProperties).toJson(QJsonDocument::Compact)).toUtf8().constData() );

			QJsonObject udpn = _originalStateProperties.value(STATE_UDPN).toObject();
			if (!udpn.isEmpty())
			{
				_originalStateUdpnSend = udpn[STATE_UDPN_SEND].toBool(false);
				_originalStateUdpnRecv = udpn[STATE_UDPN_RECV].toBool(true);
			}

			_wledInfo = response.getBody().object().value(API_PATH_INFO).toObject();
			DebugIf(verbose, _log, "info: [%s]", QString(QJsonDocument(_wledInfo).toJson(QJsonDocument::Compact)).toUtf8().constData() );

			_currentVersion.setVersion(_wledInfo.value(INFO_VER).toString().toStdString());
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

		if (_isStreamToSegment)
		{
			QJsonArray propertiesSegments = _originalStateProperties[STATE_SEG].toArray();
			QJsonArray segments;
			for (const auto& segmentItem : qAsConst(propertiesSegments))
			{
				QJsonObject segmentObj = segmentItem.toObject();

				int segmentID = segmentObj.value(STATE_SEG_ID).toInt();
				if (segmentID == _streamSegmentId)
				{
					segmentObj[STATE_ON] = _isStayOnAfterStreaming;
				}
				segments.append(segmentObj);
			}
			_originalStateProperties[STATE_SEG] = segments;
		}

		_originalStateProperties[STATE_LIVE] = false;
		_originalStateProperties[STATE_TRANSITIONTIME_CURRENTCALL] = 0;
		if (_isStayOnAfterStreaming)
		{
			_originalStateProperties[STATE_ON] = true;
		}

		httpResponse response = _restApi->put(_originalStateProperties);
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
			else
			{
				QJsonObject propertiesDetails = response.getBody().object();

				_wledInfo = propertiesDetails.value(API_PATH_INFO).toObject();
				_currentVersion.setVersion(_wledInfo.value(INFO_VER).toString().toStdString());
				if (!isReadyForDDPStreaming(_currentVersion))
				{
					if (!propertiesDetails.isEmpty())
					{
						propertiesDetails.insert("maxLedCount", UDP_MAX_LED_NUM);
					}
				}
				properties.insert("properties", propertiesDetails);
			}
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

			QJsonObject cmd;

			cmd.insert(STATE_ON, true);
			cmd.insert(STATE_LOR, 1);

			_streamSegmentId = params[CONFIG_STREAM_SEGMENT_ID].toInt(0);

			QJsonObject segment;
			segment = getSegmentObject(_streamSegmentId, true, BRI_MAX);
			segment.insert(STATE_SEG_FX, 25);
			segment.insert(STATE_SEG_SX, 128);

			QJsonArray segments;
			segments.append(segment);
			cmd.insert(STATE_SEG, segments);

			sendStateUpdateRequest(cmd,"Identify");

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
