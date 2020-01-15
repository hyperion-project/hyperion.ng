// Local-Hyperion includes
#include "LedDeviceNanoleaf.h"

// ssdp discover
#include <ssdp/SSDPDiscover.h>

// Qt includes
#include <QEventLoop>
#include <QNetworkReply>

//std includes
#include <sstream>
#include <iomanip>

//
static const bool verbose  = false;
static const bool verbose3 = false;

// Controller configuration settings
static const char CONFIG_ADDRESS[] = "output";
//static const char CONFIG_PORT[] = "port";
static const char CONFIG_AUTH_TOKEN[] ="token";

// Panel configuration settings
static const char PANEL_LAYOUT[] = "layout";
static const char PANEL_NUM[] = "numPanels";
static const char PANEL_ID[] = "panelId";
static const char PANEL_POSITIONDATA[] = "positionData";
static const char PANEL_SHAPE_TYPE[] = "shapeType";
//static const char PANEL_ORIENTATION[] = "0";
static const char PANEL_POS_X[] = "x";
static const char PANEL_POS_Y[] = "y";

// List of State Information
static const char STATE_ON[] = "on";
static const char STATE_ONOFF_VALUE[] = "value";
static const char STATE_VALUE_TRUE[] = "true";
static const char STATE_VALUE_FALSE[] = "false";

//Device Data elements
static const char DEV_DATA_NAME[] = "name";
static const char DEV_DATA_MODEL[] = "model";
static const char DEV_DATA_MANUFACTURER[] = "manufacturer";
static const char DEV_DATA_FIRMWAREVERSION[] = "firmwareVersion";

//Nanoleaf Stream Control elements
//static const char STREAM_CONTROL_IP[] = "streamControlIpAddr";
static const char STREAM_CONTROL_PORT[] = "streamControlPort";
//static const char STREAM_CONTROL_PROTOCOL[] = "streamControlProtocol";
const quint16 STREAM_CONTROL_DEFAULT_PORT = 60222; //Fixed port for Canvas;

// Nanoleaf OpenAPI URLs
static const char API_DEFAULT_PORT[] = "16021";
static const char API_URL_FORMAT[] = "http://%1:%2/api/v1/%3/%4";
static const char API_ROOT[] = "";
//static const char API_EXT_MODE_STRING_V1[] = "{\"write\" : {\"command\" : \"display\", \"animType\" : \"extControl\"}}";
static const char API_EXT_MODE_STRING_V2[] = "{\"write\" : {\"command\" : \"display\", \"animType\" : \"extControl\", \"extControlVersion\" : \"v2\"}}";
static const char API_STATE[] ="state";
static const char API_PANELLAYOUT[] = "panelLayout";
static const char API_EFFECT[] = "effects";

//Nanoleaf ssdp services
static const char SSDP_CANVAS[] = "nanoleaf:nl29";
static const char SSDP_LIGHTPANELS[] = "nanoleaf_aurora:light";
const int SSDP_TIMEOUT = 5000; // timout in ms

// Nanoleaf Panel Shapetypes
enum SHAPETYPES {
	TRIANGLE,
	RHYTM,
	SQUARE,
	CONTROL_SQUARE_PRIMARY,
	CONTROL_SQUARE_PASSIVE,
	POWER_SUPPLY,
};

// Nanoleaf external control versions
enum EXTCONTROLVERSIONS {
	EXTCTRLVER_V1 = 1,
	EXTCTRLVER_V2
};

LedDevice* LedDeviceNanoleaf::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceNanoleaf(deviceConfig);
}

LedDeviceNanoleaf::LedDeviceNanoleaf(const QJsonObject &deviceConfig)
	: ProviderUdp()
{
	_deviceReady = init(deviceConfig);
}

bool LedDeviceNanoleaf::init(const QJsonObject &deviceConfig) {

	LedDevice::init(deviceConfig);

	uint configuredLedCount = static_cast<uint>(this->getLedCount());
	Debug(_log, "DeviceType   : %s", QSTRING_CSTR( this->getActiveDeviceType() ));
	Debug(_log, "LedCount     : %u", configuredLedCount);
	Debug(_log, "ColorOrder   : %s", QSTRING_CSTR( this->getColorOrder() ));
	Debug(_log, "LatchTime    : %d", this->getLatchTime());

	//Set hostname as per configuration and default port
	_hostname   = deviceConfig[ CONFIG_ADDRESS ].toString();
	_api_port   = API_DEFAULT_PORT;
	_auth_token = deviceConfig[ CONFIG_AUTH_TOKEN ].toString();

	//If host not configured then discover device
	if ( _hostname.isEmpty() )
		//Discover Nanoleaf device
		if ( !discoverNanoleafDevice() ) {
			throw std::runtime_error("No target IP defined nor Nanoleaf device discovered");
		}

	//Get Nanoleaf device details and configuration
	_networkmanager = new QNetworkAccessManager();

	// Read Panel count and panel Ids
	QString url = getUrl(_hostname, _api_port, _auth_token, API_ROOT );
	QJsonDocument doc = getJson( url );

	QJsonObject jsonAllPanelInfo = doc.object();

	QString deviceName = jsonAllPanelInfo[DEV_DATA_NAME].toString();
	_deviceModel = jsonAllPanelInfo[DEV_DATA_MODEL].toString();
	QString deviceManufacturer = jsonAllPanelInfo[DEV_DATA_MANUFACTURER].toString();
	_deviceFirmwareVersion = jsonAllPanelInfo[DEV_DATA_FIRMWAREVERSION].toString();

	Debug(_log, "Name           : %s", QSTRING_CSTR( deviceName ));
	Debug(_log, "Model          : %s", QSTRING_CSTR( _deviceModel ));
	Debug(_log, "Manufacturer   : %s", QSTRING_CSTR( deviceManufacturer ));
	Debug(_log, "FirmwareVersion: %s", QSTRING_CSTR( _deviceFirmwareVersion));

	// Get panel details from /panelLayout/layout
	QJsonObject jsonPanelLayout = jsonAllPanelInfo[API_PANELLAYOUT].toObject();
	QJsonObject jsonLayout = jsonPanelLayout[PANEL_LAYOUT].toObject();

	uint panelNum = static_cast<uint>(jsonLayout[PANEL_NUM].toInt());
	QJsonArray positionData = jsonLayout[PANEL_POSITIONDATA].toArray();

	std::map<uint, std::map<uint, uint>> panelMap;

	// Loop over all children.
	foreach (const QJsonValue & value, positionData) {
		QJsonObject panelObj = value.toObject();

		uint panelId = static_cast<uint>(panelObj[PANEL_ID].toInt());
		uint panelX  = static_cast<uint>(panelObj[PANEL_POS_X].toInt());
		uint panelY  = static_cast<uint>(panelObj[PANEL_POS_Y].toInt());
		uint panelshapeType = static_cast<uint>(panelObj[PANEL_SHAPE_TYPE].toInt());
		//uint panelOrientation = static_cast<uint>(panelObj[PANEL_ORIENTATION].toInt());

		DebugIf(verbose, _log, "Panel [%u] (%u,%u) - Type: [%u]", panelId, panelX, panelY, panelshapeType );

		// Skip Rhythm panels
		if ( panelshapeType != RHYTM ) {
			panelMap[panelY][panelX] = panelId;
		} else  {
			Info(_log, "Rhythm panel skipped.");
		}
	}

	// Sort panels top down, left right
	for(auto posY = panelMap.crbegin(); posY != panelMap.crend(); ++posY) {
		// posY.first is the first key
		for(auto const &posX : posY->second) {
			// posX.first is the second key, posX.second is the data
			DebugIf(verbose3, _log, "panelMap[%u][%u]=%u", posY->first, posX.first, posX.second );
			_panelIds.push_back(posX.second);
		}
	}
	this->_panelLedCount = static_cast<uint>(_panelIds.size());


	Debug(_log, "PanelsNum      : %u", panelNum);
	Debug(_log, "PanelLedCount  : %u", _panelLedCount);

	// Check. if enough panelds were found.
	if (_panelLedCount < configuredLedCount) {

		throw std::runtime_error ( (QString ("Not enough panels [%1] for configured LEDs [%2] found!").arg(_panelLedCount).arg(configuredLedCount)).toStdString() );
	} else {
		if ( _panelLedCount > static_cast<uint>(this->getLedCount()) ) {
			Warning(_log, "Nanoleaf: More panels [%u] than configured LEDs [%u].", _panelLedCount, configuredLedCount );
		}
	}

	// Set UDP streaming port
	_port = STREAM_CONTROL_DEFAULT_PORT;
	_defaultHost = _hostname;

	switchOn();

	ProviderUdp::init(deviceConfig);

	Debug(_log, "Started successfully" );
	return true;
}

bool LedDeviceNanoleaf::discoverNanoleafDevice() {

	bool isDeviceFound (false);
	// device searching by ssdp
	QString address;
	SSDPDiscover discover;

	// Discover Canvas device
	address = discover.getFirstService(STY_WEBSERVER, SSDP_CANVAS, SSDP_TIMEOUT);

	//No Canvas device not found
	if ( address.isEmpty() ) {
		// Discover Light Panels (Aurora) device
		address = discover.getFirstService(STY_WEBSERVER, SSDP_LIGHTPANELS, SSDP_TIMEOUT);

		if ( address.isEmpty() ) {
			Warning(_log, "No Nanoleaf device discovered");
		}
	}

	// Canvas or Light Panels found
	if ( ! address.isEmpty() ) {
		Info(_log, "Nanoleaf device discovered at [%s]", QSTRING_CSTR( address ));
		isDeviceFound = true;
		QStringList addressparts = address.split(":", QString::SkipEmptyParts);
		_hostname = addressparts[0];
		_api_port = addressparts[1];
	}
	return isDeviceFound;
}

QJsonDocument LedDeviceNanoleaf::changeToExternalControlMode() {

	QString url = getUrl(_hostname, _api_port, _auth_token, API_EFFECT );
	QJsonDocument jsonDoc;

	_extControlVersion = EXTCTRLVER_V2;
	//Enable UDP Mode v2
	jsonDoc= putJson(url, API_EXT_MODE_STRING_V2);

	return jsonDoc;
}

QString LedDeviceNanoleaf::getUrl(QString host, QString port, QString auth_token, QString endpoint) const {
	return QString(API_URL_FORMAT).arg(host, port, auth_token, endpoint);
}

QJsonDocument LedDeviceNanoleaf::getJson(QString url) const {

	Debug(_log, "GET: [%s]", QSTRING_CSTR( url ));

	// Perfrom request
	QNetworkRequest request(url);
	QNetworkReply* reply = _networkmanager->get(request);
	// Connect requestFinished signal to quit slot of the loop.
	QEventLoop loop;
	loop.connect(reply, SIGNAL(finished()), SLOT(quit()));
	// Go into the loop until the request is finished.
	loop.exec();

	QJsonDocument jsonDoc;
	if(reply->operation() == QNetworkAccessManager::GetOperation)
	{
		jsonDoc = handleReply( reply );
	}
	// Free space.
	reply->deleteLater();
	// Return response
	return jsonDoc;
}

QJsonDocument LedDeviceNanoleaf::putJson(QString url, QString json) const {

	Debug(_log, "PUT: [%s] [%s]", QSTRING_CSTR( url ), QSTRING_CSTR( json ) );
	// Perfrom request
	QNetworkRequest request(url);
	QNetworkReply* reply = _networkmanager->put(request, json.toUtf8());
	// Connect requestFinished signal to quit slot of the loop.
	QEventLoop loop;
	loop.connect(reply, SIGNAL(finished()), SLOT(quit()));
	// Go into the loop until the request is finished.
	loop.exec();

	QJsonDocument jsonDoc;
	if(reply->operation() == QNetworkAccessManager::PutOperation)
	{
		jsonDoc = handleReply( reply );
	}
	// Free space.
	reply->deleteLater();

	// Return response
	return jsonDoc;
}

QJsonDocument LedDeviceNanoleaf::handleReply(QNetworkReply* const &reply ) const  {

	QJsonDocument jsonDoc;

	int httpStatusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
	Debug(_log, "Reply.httpStatusCode [%d]", httpStatusCode );

	if(reply->error() ==
			QNetworkReply::NoError)
	{
		if ( httpStatusCode != 204 ){
			QByteArray response = reply->readAll();
			QJsonParseError error;
			jsonDoc = QJsonDocument::fromJson(response, &error);
			if (error.error != QJsonParseError::NoError)
			{
				Error (_log, "Got invalid response");
				throw std::runtime_error("");
			}
			else {
				//Debug
				QString strJson(jsonDoc.toJson(QJsonDocument::Compact));
				DebugIf(verbose, _log, "Reply: [%s]", strJson.toUtf8().constData() );
			}
		}
	}
	else
	{
		QString errorReason;
		if ( httpStatusCode > 0 ) {
			QString httpReason = reply->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).toString();
			QString advise;
			switch ( httpStatusCode ) {
				case 400:
					advise = "Check Request Body";
					break;
				case 401:
					advise = "Check Authentication Token (API Key)";
					break;
				case 404:
					advise = "Check Resource given";
					break;
				default:
					break;
			}
			errorReason = QString ("%1:%2 [%3 %4] - %5").arg(_hostname, _api_port, QString(httpStatusCode) , httpReason);
		}
		else {
			errorReason = QString ("%1:%2 - %3").arg(_hostname, _api_port, reply->errorString());
		}
		Error (_log, "%s", QSTRING_CSTR( errorReason ));
		throw std::runtime_error("Network Error");
	}
	// Return response
	return jsonDoc;
}


LedDeviceNanoleaf::~LedDeviceNanoleaf()
{
	delete _networkmanager;
}

int LedDeviceNanoleaf::write(const std::vector<ColorRgb> & ledValues)
{

	int retVal = 0;
	uint udpBufferSize;

	//
	//    nPanels         2B
	//    panelID         2B
	//    <R> <G> <B>     3B
	//    <W>             1B
	//    tranitionTime   2B
	//
	// Note: Nanoleaf Light Panels (Aurora) now support External Control V2 (tested with FW 3.2.0)

	udpBufferSize = _panelLedCount * 8 + 2;
	std::vector<uint8_t> udpbuffer;
	udpbuffer.resize(udpBufferSize);

	uchar lowByte;  // lower byte
	uchar highByte; // upper byte

	uint i=0;

	// Set number of panels
	highByte = static_cast<uchar>(_panelLedCount >>8   );
	lowByte  = static_cast<uchar>(_panelLedCount & 0xFF);

	udpbuffer[i++] = highByte;
	udpbuffer[i++] = lowByte;

	ColorRgb color;
	for ( uint panelCounter=0; panelCounter < _panelLedCount; panelCounter++ )
	{
		uint panelID = _panelIds[panelCounter];

		highByte = static_cast<uchar>(panelID >>8   );
		lowByte  = static_cast<uchar>(panelID & 0xFF);

		// Set panels configured
		if( panelCounter < static_cast<uint>(this->getLedCount()) ) {
			color = static_cast<ColorRgb>(ledValues.at(panelCounter));
		}
		else
		{
			// Set panels not configed to black;
			color = ColorRgb::BLACK;
			DebugIf(verbose3, _log, "[%u] >= panelLedCount [%u] => Set to BLACK", panelCounter, _panelLedCount );
		}

		// Set panelID
		udpbuffer[i++] = highByte;
		udpbuffer[i++] = lowByte;

		// Set panel's color LEDs
		udpbuffer[i++] = color.red;
		udpbuffer[i++] = color.green;
		udpbuffer[i++] = color.blue;

		// Set white LED
		udpbuffer[i++] = 0; // W not set manually

		// Set transition time
		unsigned char tranitionTime = 1; // currently fixed at value 1 which corresponds to 100ms

		highByte = static_cast<uchar>(tranitionTime >>8   );
		lowByte  = static_cast<uchar>(tranitionTime & 0xFF);

		udpbuffer[i++] = highByte;
		udpbuffer[i++] = lowByte;
		DebugIf(verbose3, _log, "[%u] Color: {%u,%u,%u}", panelCounter, color.red, color.green, color.blue );

	}
	DebugIf(verbose3, _log, "UDP-Address [%s], UDP-Port [%u], udpBufferSize[%u], Bytes to send [%u]", QSTRING_CSTR(_address.toString()), _port, udpBufferSize, i);
	DebugIf(verbose3, _log, "[%s]", uint8_vector_to_hex_string(udpbuffer).c_str() );

	retVal &= writeBytes( i , udpbuffer.data());
	DebugIf(verbose3, _log, "writeBytes(): [%d]",retVal);
	return retVal;
}

QString LedDeviceNanoleaf::getOnOffRequest (bool isOn ) const {
	QString state = isOn ? STATE_VALUE_TRUE : STATE_VALUE_FALSE;
	return QString( "{\"%1\":{\"%2\":%3}}" ).arg(STATE_ON, STATE_ONOFF_VALUE, state);
}

int LedDeviceNanoleaf::switchOn() {
	Debug(_log, "switchOn()");

	// Set Nanoleaf to External Control (UDP) mode
	Debug(_log, "Set Nanoleaf to External Control (UDP) streaming mode");
	QJsonDocument responseDoc = changeToExternalControlMode();
	// Resolve port for Ligh Panels
	QJsonObject jsonStreamControllInfo = responseDoc.object();
	if ( ! jsonStreamControllInfo.isEmpty() ) {
		_port = static_cast<uchar>(jsonStreamControllInfo[STREAM_CONTROL_PORT].toInt());
	}

	//Switch on Nanoleaf device
	QString url = getUrl(_hostname, _api_port, _auth_token, API_STATE );
	putJson(url, this->getOnOffRequest(true) );

	return 0;
}

int LedDeviceNanoleaf::switchOff() {
	Debug(_log, "switchOff()");

	//Set all LEDs to Black
	int rc = writeBlack();

	//Switch off Nanoleaf device physically
	QString url = getUrl(_hostname, _api_port, _auth_token, API_STATE );
	putJson(url, getOnOffRequest(false) );

	return rc;
}

std::string LedDeviceNanoleaf:: uint8_vector_to_hex_string( const std::vector<uint8_t>& buffer ) const
{
	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	std::vector<uint8_t>::const_iterator it;

	for (it = buffer.begin(); it != buffer.end(); it++)
	{
		ss << " " << std::setw(2) << static_cast<unsigned>(*it);
	}
	return ss.str();
}
